#ifndef BINREC_REMOVE_OPT_NONE_HPP
#define BINREC_REMOVE_OPT_NONE_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class RemoveOptNonePass : public llvm::PassInfoMixin<RemoveOptNonePass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
