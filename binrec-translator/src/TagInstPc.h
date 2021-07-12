#ifndef BINREC_TAGINSTPC_H
#define BINREC_TAGINSTPC_H

#include <llvm/IR/PassManager.h>

namespace binrec {
/// S2E Add metadata annotations to PC stores at instruction boundaries
class TagInstPcPass : public llvm::PassInfoMixin<TagInstPcPass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif
