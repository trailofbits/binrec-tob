#ifndef BINREC_INTRINSIC_CLEANER_HPP
#define BINREC_INTRINSIC_CLEANER_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class IntrinsicCleanerPass : public llvm::PassInfoMixin<IntrinsicCleanerPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
