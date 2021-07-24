#ifndef BINREC_SUCCESSOR_LISTS_HPP
#define BINREC_SUCCESSOR_LISTS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Create metadata annotations for successor lists
    class SuccessorListsPass : public llvm::PassInfoMixin<SuccessorListsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
