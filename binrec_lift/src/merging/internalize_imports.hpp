#ifndef BINREC_INTERNALIZE_IMPORTS_HPP
#define BINREC_INTERNALIZE_IMPORTS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Internalize non-recovered functions for -globaldce
    class InternalizeImportsPass : public llvm::PassInfoMixin<InternalizeImportsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
