#ifndef BINREC_HALT_ON_DECLARATIONS_HPP
#define BINREC_HALT_ON_DECLARATIONS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E List any function declarations whose implementation is missing
    class HaltOnDeclarationsPass : public llvm::PassInfoMixin<HaltOnDeclarationsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
