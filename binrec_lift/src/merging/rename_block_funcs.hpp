#ifndef BINREC_RENAME_BLOCK_FUNCS_HPP
#define BINREC_RENAME_BLOCK_FUNCS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Rename captured basic blocks to Func_ prefix and remove unused blocks
    class RenameBlockFuncsPass : public llvm::PassInfoMixin<RenameBlockFuncsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
