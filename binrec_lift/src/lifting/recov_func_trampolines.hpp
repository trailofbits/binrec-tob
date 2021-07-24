#ifndef BINREC_RECOV_FUNC_TRAMPOLINES_HPP
#define BINREC_RECOV_FUNC_TRAMPOLINES_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// insert trampolines for orig binary to call recovered functions
    class RecovFuncTrampolinesPass : public llvm::PassInfoMixin<RecovFuncTrampolinesPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif // BINREC_RECOV_FUNC_TRAMPOLINES_HPP
