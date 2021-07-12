#ifndef BINREC_CALLTRACER_H
#define BINREC_CALLTRACER_H

#include <llvm/IR/PassManager.h>

namespace binrec {
class CallTracerPass : public llvm::PassInfoMixin<CallTracerPass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif
