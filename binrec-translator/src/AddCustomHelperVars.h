#ifndef BINREC_ADDCUSTOMHELPERVARS_H_H
#define BINREC_ADDCUSTOMHELPERVARS_H_H

#include <llvm/IR/PassManager.h>

namespace binrec {
/// S2E Insert variables that can be used by custom helpers
class AddCustomHelperVarsPass : public llvm::PassInfoMixin<AddCustomHelperVarsPass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif
