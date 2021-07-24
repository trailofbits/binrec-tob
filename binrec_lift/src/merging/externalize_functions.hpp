#ifndef BINREC_EXTERNALIZE_FUNCTIONS_HPP
#define BINREC_EXTERNALIZE_FUNCTIONS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Give all function definitions and declarations external linkage
    class ExternalizeFunctionsPass : public llvm::PassInfoMixin<ExternalizeFunctionsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
