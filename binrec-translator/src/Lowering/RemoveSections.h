#ifndef BINREC_REMOVESECTIONS_H
#define BINREC_REMOVESECTIONS_H

#include <llvm/IR/PassManager.h>

namespace binrec {
/// S2E Remove section globals and replace references to them with constant getelementptr instructions
class RemoveSectionsPass : public llvm::PassInfoMixin<RemoveSectionsPass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif // BINREC_REMOVESECTIONS_H
