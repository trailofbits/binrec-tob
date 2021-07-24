#ifndef BINREC_LIB_CALL_NEW_PLT_HPP
#define BINREC_LIB_CALL_NEW_PLT_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// replace fn ptr arg of helper_stub_trampoline with newplt function ptr
    class LibCallNewPLTPass : public llvm::PassInfoMixin<LibCallNewPLTPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
