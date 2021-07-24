#ifndef BINREC_GLOBAL_ENV_TO_ALLOCA_HPP
#define BINREC_GLOBAL_ENV_TO_ALLOCA_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class GlobalEnvToAllocaPass : public llvm::PassInfoMixin<GlobalEnvToAllocaPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
