#ifndef BINREC_INSERT_CALLS_HPP
#define BINREC_INSERT_CALLS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// Insert calls between recovered functions.
    class InsertCallsPass : public llvm::PassInfoMixin<InsertCallsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
