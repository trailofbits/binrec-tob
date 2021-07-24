#ifndef BINREC_IMPLEMENT_LIB_CALLS_NEW_PLT_HPP
#define BINREC_IMPLEMENT_LIB_CALLS_NEW_PLT_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// make a stub call into a full fledged call through the plt
    class ImplementLibCallsNewPLTPass : public llvm::PassInfoMixin<ImplementLibCallsNewPLTPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
