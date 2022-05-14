#ifndef BINREC_INLINEWRAPPER_HPP
#define BINREC_INLINEWRAPPER_HPP

#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

namespace binrec {
    class InlineWrapperPass : public llvm::PassInfoMixin<InlineWrapperPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

struct InlineWrapperLegacyPass : public llvm::ModulePass {
    static char ID;
    InlineWrapperLegacyPass() : ModulePass(ID) {}

    auto runOnModule(llvm::Module &m) -> bool override;
};

#endif
