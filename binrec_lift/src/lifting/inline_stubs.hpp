#ifndef BINREC_INLINE_STUBS_HPP
#define BINREC_INLINE_STUBS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Inline extern function stubs at each call to simplify CFG
    ///
    /// For each bb that has only one successor which has a "extern_symbol"
    /// annotation, copy the successor and replace the label in the successor list.
    ///
    /// Also, replace the return value of helper_extern_call in the clone with a
    /// dword load at R_ESP, so that optimization will be able to find a single
    /// successor.
    class InlineStubsPass : public llvm::PassInfoMixin<InlineStubsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
