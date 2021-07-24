#ifndef BINREC_CALL_TRACER_HPP
#define BINREC_CALL_TRACER_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class CallTracerPass : public llvm::PassInfoMixin<CallTracerPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
