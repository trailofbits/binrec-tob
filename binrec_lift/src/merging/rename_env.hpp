#ifndef BINREC_RENAME_ENV_HPP
#define BINREC_RENAME_ENV_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Rename globals from env struct to match what the next passes expect
    class RenameEnvPass : public llvm::PassInfoMixin<RenameEnvPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
