#ifndef BINREC_IMPLEMENT_LIB_CALL_STUBS_HPP
#define BINREC_IMPLEMENT_LIB_CALL_STUBS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Insert PLT jumps in library function stubs
    class ImplementLibCallStubsPass : public llvm::PassInfoMixin<ImplementLibCallStubsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
