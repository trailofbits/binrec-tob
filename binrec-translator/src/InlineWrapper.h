#ifndef BINREC_INLINEWRAPPER_H
#define BINREC_INLINEWRAPPER_H

#include <llvm/IR/PassManager.h>

namespace binrec {
class InlineWrapperPass : public llvm::PassInfoMixin<InlineWrapperPass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;
};
} // namespace binrec

struct InlineWrapperLegacyPass : public llvm::ModulePass {
    static char ID;
    InlineWrapperLegacyPass() : ModulePass(ID) {}

    auto runOnModule(llvm::Module &m) -> bool override;
};

#endif // BINREC_INLINEWRAPPER_H
