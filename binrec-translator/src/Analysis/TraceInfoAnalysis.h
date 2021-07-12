#ifndef BINREC_TRACEINFOANALYSIS_H
#define BINREC_TRACEINFOANALYSIS_H

#include "binrec/tracing/TraceInfo.h"
#include <llvm/IR/PassManager.h>

namespace binrec {
class TraceInfoAnalysis : public llvm::AnalysisInfoMixin<TraceInfoAnalysis> {
    friend llvm::AnalysisInfoMixin<TraceInfoAnalysis>;
    static llvm::AnalysisKey Key; // NOLINT

public:
    using Result = TraceInfo;
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> TraceInfo;
};
} // namespace binrec

#endif
