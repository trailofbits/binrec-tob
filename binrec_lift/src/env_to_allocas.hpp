#ifndef BINREC_ENV_TO_ALLOCAS_HPP
#define BINREC_ENV_TO_ALLOCAS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Transform environment globals into allocas in @wrapper
    class EnvToAllocasPass : public llvm::PassInfoMixin<EnvToAllocasPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
