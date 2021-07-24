#ifndef BINREC_INTERNALIZE_FUNCTIONS_HPP
#define BINREC_INTERNALIZE_FUNCTIONS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Internalize functions for removal by -globalde
    class InternalizeFunctionsPass : public llvm::PassInfoMixin<InternalizeFunctionsPass> {
    public:
        auto run(llvm::Function &m, llvm::FunctionAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
