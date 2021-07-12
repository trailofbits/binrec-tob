#include "TraceInfoAnalysis.h"
#include "PassUtils.h"
#include <fstream>

using namespace binrec;
using namespace llvm;
using namespace std;

AnalysisKey TraceInfoAnalysis::Key;

// NOLINTNEXTLINE
auto binrec::TraceInfoAnalysis::run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> TraceInfo {
    TraceInfo ti;
    ifstream f;
    s2eOpen(f, TraceInfo::defaultFilename);
    if (!f)
        return {};
    f >> ti;
    return ti;
}
