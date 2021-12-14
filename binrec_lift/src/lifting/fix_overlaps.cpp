#include "fix_overlaps.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "pc_utils.hpp"
#include <algorithm>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <set>

using namespace binrec;
using namespace llvm;
using namespace std;

static auto find_merge_pairs(Function &wrapper) -> list<pair<BasicBlock *, BasicBlock *>>
{
    list<pair<BasicBlock *, BasicBlock *>> merge_list;

    map<unsigned, vector<BasicBlock *>> bb_map;
    set<BasicBlock *> merge_blocks;

    // Put all blocks ending at the same PC in the same bucket
    for (BasicBlock &bb : wrapper) {
        if (isRecoveredBlock(&bb)) {
            unsigned lastpc = getLastPc(&bb);

            if (bb_map.find(lastpc) == bb_map.end())
                bb_map[lastpc] = vector<BasicBlock *>();

            bb_map[lastpc].push_back(&bb);
        }
    }

    // For each block in bucket (last PC of a block)
    // merge them pairwise
    // block1: [startpc            endpc]
    // block2:     [startpc        endpc]
    // block3:        [startpc     endpc] 
    // merge_blocks_same_end_point will merge
    // block1, block2 and block2, block3
    for (auto it : bb_map) {
        vector<BasicBlock *> &merge = it.second;

        if (merge.size() < 2)
            continue;

        // a block can only be merged with one other block
        for (BasicBlock *bb : merge) {
            if (merge_blocks.find(bb) != merge_blocks.end()) {
                ERROR("block " << bb->getName() << " being merged twice");
                exit(1);
            }
            merge_blocks.insert(bb);
        }

        // sort merge candidates by ascending start address
        std::sort(merge.begin(), merge.end(), [](BasicBlock *a, BasicBlock *b) {
            return getBlockAddress(a) < getBlockAddress(b);
        });

        // merge the largest two successors, then merge the result with the
        // next, etc.
        for (auto a = merge.begin(), b = a + 1; b != merge.end(); a++, b++)
            merge_list.emplace_back(*a, *b);
    }

    return merge_list;
}

// Will transform merge pairs (block1, block2) and (block2, block3)
// block1: [startpc            endpc]
// block2:     [startpc        endpc]
// block3:        [startpc     endpc] 
// into
// block1: [   ] jmp block2
// block2:     [  ] jmp block3 
// block3:        [                 ]
static void merge_blocks_same_end_point(BasicBlock *a, BasicBlock *b)
{
    // merge b into a by removing the part emulated by b from a and connecting
    // a and b with a jump
    DBG("merge blocks " << a->getName() << " and " << b->getName());

    // find split point, it is possible that splitBlock != a
    Instruction *pivot = findInstStart(a, getBlockAddress(b));

    // check whether it is the last store instruction
    Instruction *real_pivot = pivot->getNextNode();
    if (real_pivot->getOpcode() == Instruction::Store &&
        real_pivot->getOperand(1)->getName() == "cc_op") {
        DBG("realPivot's operand is " << real_pivot->getOperand(1)->getName());
        real_pivot = real_pivot->getNextNode();
    }

    assert(pivot);
    assert(real_pivot);

    BasicBlock *split_block = pivot->getParent();

    // temporarily remove all block metadata from a in case its terminator is
    // removed
    SmallVector<pair<unsigned, MDNode *>, 4> mds;
    Instruction *term = a->getTerminator();
    term->getAllMetadata(mds);
    for (auto it : mds)
        term->setMetadata(it.first, nullptr);

    // split a and insert a branch to b
    if (split_block != a)
        DBG("splitting successor block of " << a->getName());

    BasicBlock *dead_tail = split_block->splitBasicBlock(real_pivot);

    // duplicate PC store and follow it by 'ret void' to mimic regular
    // successors for recovered blocks
    auto *pc_store = cast<StoreInst>(pivot->clone());
    pc_store->setMetadata("inststart", nullptr);
    pc_store->insertBefore(split_block->getTerminator());
    split_block->getTerminator()->eraseFromParent();
    BranchInst::Create(b, split_block);
    DeleteDeadBlock(dead_tail);

    // re-insert metadata back into (possibly new) terminator
    term = a->getTerminator();
    for (auto it : mds)
        term->setMetadata(it.first, it.second);

    // update lastpc of a to the last instruction before b
    setBlockMeta(a, "lastpc", getLastInstStartPc(a));

    // copy all successors of a to b, except b itself
    vector<BasicBlock *> a_succs, b_succs;
    getBlockSuccs(a, a_succs);
    getBlockSuccs(b, b_succs);

    for (BasicBlock *succ : a_succs) {
        if (!vectorContains(b_succs, succ))
            b_succs.push_back(succ);
    }

    setBlockSuccs(b, b_succs);

    // set b as only successor of a
    a_succs.clear();
    a_succs.push_back(b);
    setBlockSuccs(a, a_succs);

    // tag blocks as merged (for plotting in -dot-merged-blocks pass)
    setBlockMeta(a, "merged", MDNode::get(a->getContext(), NULL));
    setBlockMeta(b, "merged", MDNode::get(a->getContext(), NULL));
}

static void
find_exception_overlaps(Function *wrapper, list<pair<BasicBlock *, BasicBlock *>> &merge_list)
{
    vector<BasicBlock *> succs;

    for (BasicBlock &bb : *wrapper) {
        if (isRecoveredBlock(&bb) && getBlockSuccs(&bb, succs)) {
            unsigned lastpc = getLastPc(&bb);

            for (BasicBlock *succ : succs) {
                if (succ != &bb && getBlockAddress(succ) == lastpc) {
                    merge_list.emplace_back(&bb, succ);
                    break;
                }
            }
        }
    }
}

static void remove_exception_helper(BasicBlock *bb, BasicBlock *succ)
{
    DBG("remove first instruction of " << succ->getName() << " from " << bb->getName());

    // Erase !inststart instruction and call to helper_raise_exception (the
    // rest is either required or will be removed by DCE)
    Instruction *pivot = findInstStart(bb, getBlockAddress(succ));
    assert(pivot);
    vector<Instruction *> erase_list = {pivot};

    for (auto i = pivot->getIterator(), e = pivot->getParent()->end(); i != e; i++) {
        if (auto *call = dyn_cast<CallInst>(&*i)) {
            Function *f = call->getCalledFunction();

            if (f && f->getName() == "helper_raise_exception") {
                erase_list.push_back(call);
                break;
            }
        }
    }

    // TODO (hbrodin): Any way of verifying this? Currently there seems to be a false positive
    // for blocks with a single jmp (store to @PC). They are identified in find_exception_overlaps
    // but have no call to helper_raise_exception. I consider it a false positive if the function
    // call can not be found and exit early.
    if (erase_list.size() < 2)
        return;

    for (Instruction *i : erase_list)
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

// NOLINTNEXTLINE
auto FixOverlapsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    bool changed = false;

    for (Function &f : m) {
        if (!f.getName().startswith("Func_")) {
            continue;
        }

        auto merge_list = find_merge_pairs(f);
        changed |= !merge_list.empty();

        for (auto pair : merge_list)
            merge_blocks_same_end_point(pair.first, pair.second);

        merge_list.clear();
        find_exception_overlaps(&f, merge_list);
        changed |= !merge_list.empty();

        for (auto pair : merge_list)
            remove_exception_helper(pair.first, pair.second);
    }
    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
