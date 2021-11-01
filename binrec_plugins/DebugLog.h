#ifndef __PLUGIN_DEBUGLOG_H__
#define __PLUGIN_DEBUGLOG_H__

#include <s2e/Plugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/OSMonitors/ModuleDescriptor.h>

#include <llvm/Support/raw_ostream.h>

#include "PELibCallMonitor.h"

namespace s2e {
namespace plugins {

class DebugLog : public Plugin
{
    S2E_PLUGIN
public:
    DebugLog(S2E* s2e) : Plugin(s2e) {}
    ~DebugLog() {}

    void initialize();

private:
    void slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module);
    void slotModuleExecute(S2EExecutionState *state, uint64_t pc);
    void slotModuleLeave(S2EExecutionState *state, uint64_t pc);
    void slotModuleReturn(S2EExecutionState *state, uint64_t pc);

    void slotLibCall(S2EExecutionState *state,
            const ModuleDescriptor &lib, const std::string &routine,
            uint64_t pc, PELibCallMonitor::ReturnSignal &onReturn);
    void slotLibReturn(S2EExecutionState *state, uint64_t pc);

    llvm::raw_ostream &line();
};

} // namespace plugins
} // namespace s2e

#endif // __PLUGIN_DEBUGLOG_H__
