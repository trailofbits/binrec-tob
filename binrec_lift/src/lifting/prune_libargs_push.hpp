#ifndef BINREC_PRUNE_LIBARGS_PUSH_HPP
#define BINREC_PRUNE_LIBARGS_PUSH_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Remove stores to the stack before __stub_XXX calls
    class PruneLibargsPushPass : public llvm::PassInfoMixin<PruneLibargsPushPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
