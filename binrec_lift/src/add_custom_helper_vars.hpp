#ifndef BINREC_ADDCUSTOMHELPERVARS_H_HPP
#define BINREC_ADDCUSTOMHELPERVARS_H_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Insert variables that can be used by custom helpers
    class AddCustomHelperVarsPass : public llvm::PassInfoMixin<AddCustomHelperVarsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
