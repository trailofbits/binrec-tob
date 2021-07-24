#ifndef BINREC_PRUNE_NULL_SUCCS_HPP
#define BINREC_PRUNE_NULL_SUCCS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Remove any null entries from successor lists
    ///
    /// remove libc setup/teardown code:
    /// - remove all BBs whose addresses are not in the whitelisted main-addrs file
    class PruneNullSuccsPass : public llvm::PassInfoMixin<PruneNullSuccsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
