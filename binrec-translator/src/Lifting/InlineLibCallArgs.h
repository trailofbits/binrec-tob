#ifndef BINREC_INLINELIBCALLARGS_H
#define BINREC_INLINELIBCALLARGS_H

#include <llvm/IR/PassManager.h>

namespace binrec {
/// S2E Inline arguments of library function calls with known signatures
class InlineLibCallArgsPass : public llvm::PassInfoMixin<InlineLibCallArgsPass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif // BINREC_INLINELIBCALLARGS_H
