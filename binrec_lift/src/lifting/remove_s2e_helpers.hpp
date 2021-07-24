#ifndef BINREC_REMOVE_S2E_HELPERS_HPP
#define BINREC_REMOVE_S2E_HELPERS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Remove Qemu helper functions
    class RemoveS2EHelpersPass : public llvm::PassInfoMixin<RemoveS2EHelpersPass> {
    public:
        /// - remove calls to helper_{s2e_tcg_execution_handler,fninit,lock,unlock}
        /// - replace calls to tcg_llvm_fork_and_concretize with the first
        ///   operand(since we have no symbolic values), which can then be
        ///   optimized away by constant propagation
        ///
        /// Remove stores to:
        /// - @s2e_icount_after_tb (instruction counter)
        /// - @s2e_current_tb      (translation block pointer)
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
