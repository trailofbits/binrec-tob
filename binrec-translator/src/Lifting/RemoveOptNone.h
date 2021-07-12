#ifndef BINREC_REMOVEOPTNONE_H
#define BINREC_REMOVEOPTNONE_H

#include <llvm/IR/PassManager.h>

namespace binrec {
class RemoveOptNonePass : public llvm::PassInfoMixin<RemoveOptNonePass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif // BINREC_REMOVEOPTNONE_H
