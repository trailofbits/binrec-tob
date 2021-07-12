#ifndef BINREC_SETDATALAYOUT32_H
#define BINREC_SETDATALAYOUT32_H

#include <llvm/IR/PassManager.h>

namespace binrec {
class SetDataLayout32Pass : public llvm::PassInfoMixin<SetDataLayout32Pass> {
public:
    // NOLINTNEXTLINE
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif
