#ifndef BINREC_TRACE_INFO_ANALYSIS_HPP
#define BINREC_TRACE_INFO_ANALYSIS_HPP

#include "binrec/tracing/trace_info.hpp"
#include <llvm/IR/PassManager.h>

namespace binrec {
    class TraceInfoAnalysis : public llvm::AnalysisInfoMixin<TraceInfoAnalysis> {
        friend llvm::AnalysisInfoMixin<TraceInfoAnalysis>;
        static llvm::AnalysisKey Key; // NOLINT

    public:
        using Result = TraceInfo;
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> TraceInfo;
    };
} // namespace binrec

#endif
