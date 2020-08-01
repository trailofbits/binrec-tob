#ifndef __PLUGIN_FUNCTIONLOG_H__
#define __PLUGIN_FUNCTIONLOG_H__

#include <vector>
#include <map>
#include <set>
#include <stack>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include <s2e/Plugin.h>
#include <s2e/ConfigFile.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/FunctionMonitor.h>
#include <s2e/Plugins/ModuleDescriptor.h>
#include <s2e/Plugins/ModuleExecutionDetector.h>

//#include "Export.h"

namespace s2e {
namespace plugins {

using namespace llvm;

/**
 *  Base class for binary file code selectors
 */
class FunctionLog : public Plugin
{
    S2E_PLUGIN
public:
    FunctionLog(S2E* s2e): Plugin(s2e) {}

#ifdef TARGET_I386
    void initialize();
    ~FunctionLog();
//    void slotStateFork(S2EExecutionState *state,
//            const std::vector<S2EExecutionState*> &newStates,
//            const std::vector<klee::ref<klee::Expr> > &newCondition);
private:
    std::ofstream m_logFile;
//    sigc::connection m_connExec, m_connFork;
    FunctionMonitor *m_functionMonitor;
    bool doExport;
    bool logEnabled;
    uint64_t executedBBPc;
    uint64_t callerPc;
    //pair< func_start_pc, return_pc | caller_pc >
    std::set<uint64_t> entryPcToCallerPc;
    std::set<uint64_t> entryPcToReturnPc;
    std::set<uint64_t> callerPcToRetFollowUp; //callerPc is the actual pc(not BB pc), FollowUp is bb pc
    std::map<uint32_t, std::set<uint32_t> > entryPcToBBPcs;
    std::stack<uint32_t> callStack; 

    void onFunctionCall(S2EExecutionState* state, FunctionMonitorState *fns);
    void onFunctionReturn(S2EExecutionState *state, uint64_t func_caller, uint64_t func_begin);
    void slotModuleLeave(S2EExecutionState *state, uint64_t pc);
    void slotModuleReturn(S2EExecutionState *state, uint64_t pc);
    void slotModuleExecute(S2EExecutionState* state, uint64_t pc);
    
    void slotModuleLoad(S2EExecutionState *state,
            const ModuleDescriptor &module);
    void slotModuleSignal(S2EExecutionState *state,
            const ModuleExecutionCfg &desc);
#endif
};

} // namespace plugins
} // namespace s2e

#endif // __PLUGIN_FUNCTIONLOG_H__
