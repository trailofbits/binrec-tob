#ifndef BINREC_INLINE_LIB_CALL_ARGS_HPP
#define BINREC_INLINE_LIB_CALL_ARGS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Inline arguments of library function calls with known signatures
    class InlineLibCallArgsPass : public llvm::PassInfoMixin<InlineLibCallArgsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
