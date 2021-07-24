#ifndef BINREC_RECOVER_FUNCTIONS_HPP
#define BINREC_RECOVER_FUNCTIONS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// Merge translations blocks that belong to the same function.
    class RecoverFunctionsPass : public llvm::PassInfoMixin<RecoverFunctionsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
