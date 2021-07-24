#ifndef BINREC_INTERNALIZE_DEBUG_HELPERS_HPP
#define BINREC_INTERNALIZE_DEBUG_HELPERS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Internalize debug helper functions for removal by -globalde
    class InternalizeDebugHelpersPass : public llvm::PassInfoMixin<InternalizeDebugHelpersPass> {
    public:
        auto run(llvm::Function &f, llvm::FunctionAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
