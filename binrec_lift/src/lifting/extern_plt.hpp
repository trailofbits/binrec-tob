#ifndef BINREC_EXTERN_PLT_HPP
#define BINREC_EXTERN_PLT_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Patch extern function calls to use the existing PLT
    ///
    /// Assume the presence of a function "helper_extern_call" that takes a function
    /// pointer argument and:
    /// - reads and saves a return PC from///@R_ESP
    /// - backs up concrete registers
    /// - copies virtual registers to concrete registers
    /// - calls the external function in the first argument
    /// - copies concrete values to virtual registers
    /// - restores original concrete registers
    /// - returns the saved return PC
    class ExternPLTPass : public llvm::PassInfoMixin<ExternPLTPass> {
    public:
        auto run(llvm::Function &f, llvm::FunctionAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
