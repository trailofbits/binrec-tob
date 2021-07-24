#ifndef BINREC_FIX_CFG_HPP
#define BINREC_FIX_CFG_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// Fix cfg by finding escaping pointers.
    ///
    /// This pass works only if library functions that take local pointer are reached by call
    /// instruction. If library function is reached by jmp instruction, this pass will fail.
    class FixCFGPass : public llvm::PassInfoMixin<FixCFGPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
