#ifndef BINREC_PRUNE_RETADDR_PUSH_HPP
#define BINREC_PRUNE_RETADDR_PUSH_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Remove stored of return address to the stack at libcalls
    class PruneRetaddrPushPass : public llvm::PassInfoMixin<PruneRetaddrPushPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif // BINREC_PRUNE_RETADDR_PUSH_HPP
