#ifndef BINREC_UNFLATTEN_ENV_HPP
#define BINREC_UNFLATTEN_ENV_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Convert pointer offsets to env struct member accesses
    class UnflattenEnvPass : public llvm::PassInfoMixin<UnflattenEnvPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
