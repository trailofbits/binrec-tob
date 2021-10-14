#include <cassert>

#include <s2e/S2E.h>
#include <s2e/Utils.h>

#include "DebugLog.h"
#include "ModuleSelector.h"

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(DebugLog,
                  "Log execution events to debug log",
                  "DebugLog", "ModuleSelector");

void DebugLog::initialize()
{
    ModuleSelector *selector =
        (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));
    selector->onModuleLoad.connect(
            sigc::mem_fun(*this, &DebugLog::slotModuleLoad));
    selector->onModuleExecute.connect(
            sigc::mem_fun(*this, &DebugLog::slotModuleExecute));
    selector->onModuleLeave.connect(
            sigc::mem_fun(*this, &DebugLog::slotModuleLeave));
    selector->onModuleReturn.connect(
            sigc::mem_fun(*this, &DebugLog::slotModuleReturn));

    PELibCallMonitor *mon =
        (PELibCallMonitor*)(s2e()->getPlugin("PELibCallMonitor"));
    if (mon)
        mon->onLibCall.connect(sigc::mem_fun(*this, &DebugLog::slotLibCall));
}

void DebugLog::slotModuleLoad(S2EExecutionState *state,
        const ModuleDescriptor &module) {
    line() << "module load " << module.Name << "\n";
}

void DebugLog::slotModuleExecute(S2EExecutionState *state, uint64_t pc) {
    line() << "module execute " << hexval(pc) << "\n";
}

void DebugLog::slotModuleLeave(S2EExecutionState *state, uint64_t pc) {
    line() << "module leave " << hexval(pc) << "\n";
}

void DebugLog::slotModuleReturn(S2EExecutionState *state, uint64_t pc) {
    line() << "module return\n";
}

void DebugLog::slotLibCall(S2EExecutionState *state,
        const ModuleDescriptor &lib, const std::string &routine,
        uint64_t pc, PELibCallMonitor::ReturnSignal &onReturn)
{
    line() << "libcall " << lib.Name << "::" << routine << "\n";
    onReturn.connect(sigc::mem_fun(*this, &DebugLog::slotLibReturn));
}

void DebugLog::slotLibReturn(S2EExecutionState *state, uint64_t pc)
{
    line() << "libreturn\n";
}

llvm::raw_ostream &DebugLog::line() {
    llvm::raw_ostream &s = s2e()->getDebugStream();
    s << "[DebugLog] ";
    return s;
}

} // namespace plugins
} // namespace s2e
