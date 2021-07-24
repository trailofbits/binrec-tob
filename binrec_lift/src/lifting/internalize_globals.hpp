#ifndef BINREC_INTERNALIZE_GLOBALS_HPP
#define BINREC_INTERNALIZE_GLOBALS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Internalize and zero-initialize global variables
    class InternalizeGlobalsPass : public llvm::PassInfoMixin<InternalizeGlobalsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
