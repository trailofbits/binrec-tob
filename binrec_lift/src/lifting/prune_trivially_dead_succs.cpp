#include "error.hpp"
#include "prune_trivially_dead_succs.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/CFG.h>
#include <set>

using namespace binrec;
using namespace llvm;
using namespace std;

static void find_terminating_blocks(BasicBlock *bb, set<BasicBlock *> &terms)
{
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
        find_terminating_blocks(succ, terms);
    }
}

static auto is_pc_store(Instruction &i) -> bool
{
    if (auto *store = dyn_cast<StoreInst>(&i)) {
        if (auto *glob = dyn_cast<GlobalVariable>(store->getPointerOperand()))
            return glob->hasName() && glob->getName() == "PC";
    }

    return false;
}

static auto get_const_pc_store(Instruction &i) -> unsigned
{
    if (auto *c = dyn_cast<ConstantInt>(cast<StoreInst>(&i)->getValueOperand()))
        return c->getZExtValue();

    return 0;
}

static auto find_successor(vector<BasicBlock *> &bbs, unsigned pc) -> BasicBlock *
{
    for (BasicBlock *succ : bbs) {
        if (getBlockAddress(succ) == pc)
            return succ;
    }

    return nullptr;
}


// NOLINTNEXTLINE
auto PruneTriviallyDeadSuccsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    size_t pruned_succs = 0;

    for (Function &f : m) {
        if (!f.getName().startswith("Func_"))
            continue;

        for (BasicBlock &bb : f) {
            if (!isRecoveredBlock(&bb))
                continue;

            set<BasicBlock *> terminators;
            find_terminating_blocks(&bb, terminators);

            for (BasicBlock *term : terminators) {
                unsigned last_stored_pc = 0;

                for (Instruction &i : *term) {
                    if (is_pc_store(i))
                        last_stored_pc = get_const_pc_store(i);
                }

                if (!last_stored_pc)
                    continue;

                // We know the last stored PC, assert that is in the current
                // successor list and remove everything else from the list
                vector<BasicBlock *> succs;
                getBlockSuccs(&bb, succs);
                BasicBlock *only_succ = find_successor(succs, last_stored_pc);

                if (!only_succ) {
                    LLVM_ERROR(error)
                        << "block "
                        << bb.getName() << " stores PC " << last_stored_pc
                        << " but does not have BB_" << utohexstr(last_stored_pc)
                        << " in its successor list. Did you remember to disable multithreading in "
                           "qemu (-smp 1)";
                    throw std::runtime_error{error};
                }

                if (succs.size() == 1)
                    continue;

                pruned_succs += succs.size() - 1;
                succs.clear();
                succs.push_back(only_succ);
                setBlockSuccs(&bb, succs);

                DBG("set only successor of " << bb.getName() << " to " << only_succ->getName());
            }
        }

        INFO("pruned " << pruned_succs << " trivially dead references from successor lists");
    }

    return pruned_succs > 0 ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
