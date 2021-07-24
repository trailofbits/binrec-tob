#ifndef BINREC_REMOVE_SECTIONS_HPP
#define BINREC_REMOVE_SECTIONS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Remove section globals and replace references to them with constant getelementptr
    /// instructions
    class RemoveSectionsPass : public llvm::PassInfoMixin<RemoveSectionsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
