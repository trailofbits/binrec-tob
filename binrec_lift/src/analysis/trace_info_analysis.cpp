#include "trace_info_analysis.hpp"
#include "pass_utils.hpp"
#include <fstream>

using namespace binrec;
using namespace llvm;
using namespace std;

AnalysisKey TraceInfoAnalysis::Key;

// NOLINTNEXTLINE
auto binrec::TraceInfoAnalysis::run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> TraceInfo
{
    TraceInfo ti;
    ifstream f;
    s2eOpen(f, TraceInfo::defaultFilename);
    if (!f)
        return {};
    f >> ti;
    return ti;
}
