#ifndef BINREC_UNIMPLEMENT_CUSTOM_HELPERS_HPP
#define BINREC_UNIMPLEMENT_CUSTOM_HELPERS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Remove implementations for custom helpers (from custom-helpers.bc) to prepare for
    /// linking
    class UnimplementCustomHelpersPass : public llvm::PassInfoMixin<UnimplementCustomHelpersPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
