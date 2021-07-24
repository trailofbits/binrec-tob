#ifndef BINREC_FIX_OVERLAPS_HPP
#define BINREC_FIX_OVERLAPS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E fix overlapping PC in basic blocks
    class FixOverlapsPass : public llvm::PassInfoMixin<FixOverlapsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
