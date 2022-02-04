#include "prune_redundant_basic_blocks.hpp"
#include "ir/selectors.hpp"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto PruneRedundantBasicBlocksPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{

    for (auto &f : m) {
        if (!is_lifted_function(f))
            continue;


        SmallVector<BasicBlock *, 4> drop;
        for (auto &bb : f) {
            if (&bb == &f.getEntryBlock()) {
                // After making register accesses loads/stores to global variables
                // the only remaining instruction on some entry blocks is a branch
                // to the next block. For those cases, we drop the entry block.
                if (auto branch = dyn_cast<BranchInst>(bb.begin())) {
                    if (branch->isUnconditional()) {
                        drop.push_back(&bb);
                    }
                }
            } else {
                // TODO (hbrodin): Consider 	llvm::EliminateUnreachableBlocks (Function &F,
                // DomTreeUpdater *DTU=nullptr, bool KeepOneInputPHIs=false)
                if (!bb.hasNPredecessorsOrMore(1)) {
                    drop.push_back(&bb);
                }
            }
        }

        for (auto bb : drop) {
            bb->eraseFromParent();
        }

        // Ensure entry blocks are labeled as before
        f.getEntryBlock().setName("entry");
    }

    // TODO (hbrodin): What analysis are actually preserved?
    return PreservedAnalyses::none();
}
