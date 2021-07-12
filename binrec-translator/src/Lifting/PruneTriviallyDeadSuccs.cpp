#include "MetaUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/CFG.h>
#include <llvm/IR/PassManager.h>
#include <set>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
void findTerminatingBlocks(BasicBlock *bb, set<BasicBlock *> &terms) {
    if (isa<ReturnInst>(bb->getTerminator())) {
        terms.insert(bb);
        return;
    }
    if (auto *branch = dyn_cast<BranchInst>(bb->getTerminator())) {
        if (branch->isUnconditional() && isRecoveredBlock(branch->getSuccessor(0))) {
            terms.insert(bb);
            return;
        }
    }
    for (BasicBlock *succ : successors(bb)) {
        // Case added for instructions such as
        // `rep movsd dword es:[edi], dword ptr [esi]`
        // where the BB is a successor of itself.
        if (getBlockAddress(succ) == getBlockAddress(bb)) {
            return;
        }
        findTerminatingBlocks(succ, terms);
    }
}

auto isPcStore(Instruction &i) -> bool {
    if (auto *store = dyn_cast<StoreInst>(&i)) {
        if (auto *glob = dyn_cast<GlobalVariable>(store->getPointerOperand()))
            return glob->hasName() && glob->getName() == "PC";
    }

    return false;
}

auto getConstPcStore(Instruction &i) -> unsigned {
    if (auto *c = dyn_cast<ConstantInt>(cast<StoreInst>(&i)->getValueOperand()))
        return c->getZExtValue();

    return 0;
}

auto findSuccessor(vector<BasicBlock *> &bbs, unsigned pc) -> BasicBlock * {
    for (BasicBlock *succ : bbs) {
        if (getBlockAddress(succ) == pc)
            return succ;
    }

    return nullptr;
}

/// S2E Prune edges that are trivially unreachable from successor lists
class PruneTriviallyDeadSuccsPass : public PassInfoMixin<PruneTriviallyDeadSuccsPass> {
public:
    /// Find basic blocks with successor lists that end with 'ret void' and check
    /// if the last stored PC is constant. If so, remove other successors from
    /// the successor list (and log the event).
    // NOLINTNEXTLINE
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        int prunedSuccs = 0;

        for (Function &f : m) {
            if (!f.getName().startswith("Func_"))
                continue;

            for (BasicBlock &bb : f) {
                if (!isRecoveredBlock(&bb))
                    continue;

                set<BasicBlock *> terminators;
                findTerminatingBlocks(&bb, terminators);

                for (BasicBlock *term : terminators) {
                    unsigned lastStoredPc = 0;

                    for (Instruction &i : *term) {
                        if (isPcStore(i))
                            lastStoredPc = getConstPcStore(i);
                    }

                    if (!lastStoredPc)
                        continue;

                    // We know the last stored PC, assert that is in the current
                    // successor list and remove everything else from the list
                    vector<BasicBlock *> succs;
                    getBlockSuccs(&bb, succs);
                    BasicBlock *onlySucc = findSuccessor(succs, lastStoredPc);

                    if (!onlySucc) {
                        ERROR("block " << bb.getName() << " stores PC " << lastStoredPc << " but does not have BB_"
                                       << intToHex(lastStoredPc)
                                       << " in its successor list. Did you remember to disable multithreading in "
                                          "qemu (-smp 1)");
                        exit(1);
                    }

                    if (succs.size() == 1)
                        continue;

                    prunedSuccs += succs.size() - 1;
                    succs.clear();
                    succs.push_back(onlySucc);
                    setBlockSuccs(&bb, succs);

                    DBG("set only successor of " << bb.getName() << " to " << onlySucc->getName());
                }
            }

            INFO("pruned " << prunedSuccs << " trivially dead references from successor lists");
        }

        return prunedSuccs > 0 ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addPruneTriviallyDeadSuccsPass(ModulePassManager &mpm) { mpm.addPass(PruneTriviallyDeadSuccsPass()); }
