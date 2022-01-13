#ifndef BINREC_PRUNE_REDUNDANT_BASIC_BLOCKS_HPP
#define BINREC_PRUNE_REDUNDANT_BASIC_BLOCKS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    // Removes redundant basic blocks.
    // At some stages, there will be blocks with no predecessors or blocks with only
    // a single unconditional branch to next. 
    // This is done during the cleaning phase (before reconstructing actual functions)
    // TODO (hbrodin): Could be written as a function pass instead.
    class PruneRedundantBasicBlocksPass : public llvm::PassInfoMixin<PruneRedundantBasicBlocksPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif