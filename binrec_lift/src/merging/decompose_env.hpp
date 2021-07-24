#ifndef BINREC_DECOMPOSE_ENV_HPP
#define BINREC_DECOMPOSE_ENV_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// Replace CPU state struct members with global variables
    class DecomposeEnvPass : public llvm::PassInfoMixin<DecomposeEnvPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
