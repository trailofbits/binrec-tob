#ifndef BINREC_UNALIGN_STACK_HPP
#define BINREC_UNALIGN_STACK_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Remove stack alignment of R_ESP
    class UnalignStackPass : public llvm::PassInfoMixin<UnalignStackPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
