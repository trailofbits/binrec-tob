#include "MetaUtils.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include <algorithm>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <set>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
auto findMergePairs(Function &wrapper) -> list<pair<BasicBlock *, BasicBlock *>> {
    list<pair<BasicBlock *, BasicBlock *>> mergeList;

    map<unsigned, vector<BasicBlock *>> bbMap;
    set<BasicBlock *> mergeBlocks;

    for (BasicBlock &bb : wrapper) {
        if (isRecoveredBlock(&bb)) {
            unsigned lastpc = getLastPc(&bb);

            if (bbMap.find(lastpc) == bbMap.end())
                bbMap[lastpc] = vector<BasicBlock *>();

            bbMap[lastpc].push_back(&bb);
        }
    }

    for (auto it : bbMap) {
        vector<BasicBlock *> &merge = it.second;

        if (merge.size() < 2)
            continue;

        // a block can only be merged with one other block
        for (BasicBlock *bb : merge) {
            if (mergeBlocks.find(bb) != mergeBlocks.end()) {
                ERROR("block " << bb->getName() << " being merged twice");
                exit(1);
            }
            mergeBlocks.insert(bb);
        }

        // sort merge candidates by ascending start address
        std::sort(merge.begin(), merge.end(),
                  [](BasicBlock *a, BasicBlock *b) { return getBlockAddress(a) < getBlockAddress(b); });

        // merge the largest two successors, then merge the result with the
        // next, etc.
        for (auto a = merge.begin(), b = a + 1; b != merge.end(); a++, b++)
            mergeList.emplace_back(*a, *b);
    }

    return mergeList;
}

void mergeBlocksSameEndPoint(BasicBlock *a, BasicBlock *b) {
    // merge b into a by removing the part emulated by b from a and connecting
    // a and b with a jump
    DBG("merge blocks " << a->getName() << " and " << b->getName());

    // find split point, it is possible that splitBlock != a
    Instruction *pivot = findInstStart(a, getBlockAddress(b));

    // check whether it is the last store instruction
    Instruction *realPivot = pivot->getNextNode();
    if (realPivot->getOpcode() == Instruction::Store && realPivot->getOperand(1)->getName() == "cc_op") {
        DBG("realPivot's operand is " << realPivot->getOperand(1)->getName());
        realPivot = realPivot->getNextNode();
    }

    assert(pivot);
    assert(realPivot);

    BasicBlock *splitBlock = pivot->getParent();

    // temporarily remove all block metadata from a in case its terminator is
    // removed
    SmallVector<pair<unsigned, MDNode *>, 4> mds;
    Instruction *term = a->getTerminator();
    term->getAllMetadata(mds);
    for (auto it : mds)
        term->setMetadata(it.first, nullptr);

    // split a and insert a branch to b
    if (splitBlock != a)
        DBG("splitting successor block of " << a->getName());

    BasicBlock *deadTail = splitBlock->splitBasicBlock(realPivot);

    // duplicate PC store and follow it by 'ret void' to mimic regular
    // successors for recovered blocks
    auto *pcStore = cast<StoreInst>(pivot->clone());
    pcStore->setMetadata("inststart", nullptr);
    pcStore->insertBefore(splitBlock->getTerminator());
    splitBlock->getTerminator()->eraseFromParent();
    BranchInst::Create(b, splitBlock);
    DeleteDeadBlock(deadTail);

    // re-insert metadata back into (possibly new) terminator
    term = a->getTerminator();
    for (auto it : mds)
        term->setMetadata(it.first, it.second);

    // update lastpc of a to the last instruction before b
    setBlockMeta(a, "lastpc", getLastInstStartPc(a));

    // copy all successors of a to b, except b itself
    vector<BasicBlock *> aSuccs, bSuccs;
    getBlockSuccs(a, aSuccs);
    getBlockSuccs(b, bSuccs);

    for (BasicBlock *succ : aSuccs) {
        if (!vectorContains(bSuccs, succ))
            bSuccs.push_back(succ);
    }

    setBlockSuccs(b, bSuccs);

    // set b as only successor of a
    aSuccs.clear();
    aSuccs.push_back(b);
    setBlockSuccs(a, aSuccs);

    // tag blocks as merged (for plotting in -dot-merged-blocks pass)
    setBlockMeta(a, "merged", MDNode::get(a->getContext(), NULL));
    setBlockMeta(b, "merged", MDNode::get(a->getContext(), NULL));
}

void findExceptionOverlaps(Function *wrapper, list<pair<BasicBlock *, BasicBlock *>> &mergeList) {
    vector<BasicBlock *> succs;

    for (BasicBlock &bb : *wrapper) {
        if (isRecoveredBlock(&bb) && getBlockSuccs(&bb, succs)) {
            unsigned lastpc = getLastPc(&bb);

            for (BasicBlock *succ : succs) {
                if (succ != &bb && getBlockAddress(succ) == lastpc) {
                    mergeList.emplace_back(&bb, succ);
                    break;
                }
            }
        }
    }
}

void removeExceptionHelper(BasicBlock *bb, BasicBlock *succ) {
    DBG("remove first instruction of " << succ->getName() << " from " << bb->getName());

    // Erase !inststart instruction and call to helper_raise_exception (the
    // rest is either required or will be removed by DCE)
    Instruction *pivot = findInstStart(bb, getBlockAddress(succ));
    assert(pivot);
    vector<Instruction *> eraseList = {pivot};

    for (auto i = pivot->getIterator(), e = pivot->getParent()->end(); i != e; i++) {
        if (auto *call = dyn_cast<CallInst>(&*i)) {
            Function *f = call->getCalledFunction();

            if (f && f->getName() == "helper_raise_exception") {
                eraseList.push_back(call);
                break;
            }
        }
    }

    for (Instruction *i : eraseList)
        i->eraseFromParent();

    // Update lastpc metadata to previous !inststart
    setBlockMeta(bb, "lastpc", getLastInstStartPc(bb, true));

    // Transfer successors of bb to succ and set bb's only successor to succ
    vector<BasicBlock *> succs1, succs2;
    getBlockSuccs(bb, succs1);
    getBlockSuccs(succ, succs2);

    for (BasicBlock *b : succs1) {
        if (b != succ && !vectorContains(succs2, b))
            succs2.push_back(b);
    }

    succs1.clear();
    succs1.push_back(succ);

    setBlockSuccs(bb, succs1);
    setBlockSuccs(succ, succs2);
}

/// S2E fix overlapping PC in basic blocks
class FixOverlapsPass : public PassInfoMixin<FixOverlapsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        bool Changed = false;

        for (Function &F : m) {
            if (!F.getName().startswith("Func_")) {
                continue;
            }

            bool changed = false;

            auto mergeList = findMergePairs(F);
            changed |= !mergeList.empty();

            for (auto pair : mergeList)
                mergeBlocksSameEndPoint(pair.first, pair.second);

            mergeList.clear();
            findExceptionOverlaps(&F, mergeList);
            changed |= !mergeList.empty();

            for (auto pair : mergeList)
                removeExceptionHelper(pair.first, pair.second);
        }

        return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addFixOverlapsPass(ModulePassManager &mpm) { mpm.addPass(FixOverlapsPass()); }
