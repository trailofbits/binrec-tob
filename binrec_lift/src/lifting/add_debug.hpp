#ifndef BINREC_ADD_DEBUG_HPP
#define BINREC_ADD_DEBUG_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Add debug print statements for PC values and illegal states
    ///
    /// Assume the presence of a parameter-less function "helper_extern_call" that
    /// that uses @R_ESP and @PC:
    /// 1. reads a return address from///@R_ESP
    /// 3. saves virtual values in concrete registers
    /// 4. calls the external function at @PC
    /// 5. saves concrete values in virtual registers
    /// 6. jumps to the saved return address
    class AddDebugPass : public llvm::PassInfoMixin<AddDebugPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
