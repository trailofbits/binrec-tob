#ifndef BINREC_NAME_CLEANER_HPP
#define BINREC_NAME_CLEANER_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class NameCleaner : public llvm::PassInfoMixin<NameCleaner> {
    public:
        auto run(llvm::Function &f, llvm::FunctionAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
