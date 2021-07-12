#ifndef BINREC_GLOBALENVTOALLOCA_H
#define BINREC_GLOBALENVTOALLOCA_H

#include <llvm/IR/PassManager.h>

namespace binrec {
class GlobalEnvToAllocaPass : public llvm::PassInfoMixin<GlobalEnvToAllocaPass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif
