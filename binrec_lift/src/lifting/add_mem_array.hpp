#ifndef BINREC_ADD_MEM_ARRAY_HPP
#define BINREC_ADD_MEM_ARRAY_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Inline arguments of library function calls with known signatures
    class AddMemArrayPass : public llvm::PassInfoMixin<AddMemArrayPass> {
    public:
        /// Replace calls to __ldX_mmu/__stX_mmu helpers with loads/stores to an
        /// element pointer in @memory:
        /// - %x = __ldX_mmu(addr, 1)  -->  %memptr = getelementptr @memory, addr
        ///                                 %ptr = bitcast i8* %memptr to iX*
        ///                                 %x = load %ptr
        /// - __stX_mmu(addr, %x, 1)   -->  %memptr = getelementptr @memory, addr
        ///                                 %ptr = bitcast i8* %memptr to iX*
        ///                                 store %x, %ptr
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
