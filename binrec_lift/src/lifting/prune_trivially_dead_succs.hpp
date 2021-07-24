#ifndef BINREC_PRUNE_TRIVIALLY_DEAD_SUCCS_HPP
#define BINREC_PRUNE_TRIVIALLY_DEAD_SUCCS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Prune edges that are trivially unreachable from successor lists
    ///
    /// Find basic blocks with successor lists that end with 'ret void' and check if the last stored
    /// PC is constant. If so, remove other successors from the successor list (and log the event).
    class PruneTriviallyDeadSuccsPass : public llvm::PassInfoMixin<PruneTriviallyDeadSuccsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
