#include <s2e/S2E.h>
#include <s2e/S2EExecutor.h>
#include <s2e/Plugins/ModuleExecutionDetector.h>
#include <s2e/Plugins/WindowsInterceptor/WindowsMonitor.h>

#include "PESelector.h"

namespace s2e {
namespace plugins {

using namespace llvm;

S2E_DEFINE_PLUGIN(PESelector,
                  "Emits execution events only for blocks in a particular PE binary",
                  "ModuleSelector", "ModuleExecutionDetector");

void PESelector::initialize()
{
    m_moduleNames = s2e()->getConfig()->getStringList(getConfigKey() + ".moduleNames");
    m_moduleLoaded = false;

    ModuleExecutionDetector *modEx =
        (ModuleExecutionDetector*)(s2e()->getPlugin("ModuleExecutionDetector"));
    WindowsMonitor *windowsMonitor =
        (WindowsMonitor*)(s2e()->getPlugin("Interceptor"));

    modEx->onModuleLoad.connect(
            sigc::mem_fun(*this, &PESelector::slotModuleLoad));
    modEx->onModuleTranslateBlockStart.connect(
            sigc::mem_fun(*this, &PESelector::slotModuleTranslateBlockStart));
    windowsMonitor->onProcessUnload.connect(
            sigc::mem_fun(*this, &PESelector::slotProcessUnload));

    s2e()->getMessagesStream() << "[PESelector] Plugin initialized\n";
}

void PESelector::slotModuleLoad(S2EExecutionState *state,
                                const ModuleDescriptor &module)
{
    if (isInterestingModule(module)) {
        assert(!m_moduleLoaded && "second interesting module loaded");
        m_moduleLoaded = true;
        m_moduleDesc = module;
        s2e()->getDebugStream() << "[PESelector] main module loaded\n";
        onModuleLoad.emit(state, m_moduleDesc);
    }
}

bool PESelector::isInterestingModule(const ModuleDescriptor &module)
{
    foreach2(it, m_moduleNames.begin(), m_moduleNames.end()) {
        const std::string &name = *it;
        if (name == module.Name)
            return true;
    }
    return false;
}

void PESelector::slotModuleTranslateBlockStart(
        ExecutionSignal *signal,
        S2EExecutionState *state,
        const ModuleDescriptor &module,
        TranslationBlock *tb,
        uint64_t pc)
{
    if (m_moduleLoaded && module.Pid == m_moduleDesc.Pid) {
        // FIXME: ntdll screws up libcall detection, so ignore it for now
        if (module.Name == "ntdll.dll")
            return;

        if (module.Name == m_moduleDesc.Name)
            signal->connect(sigc::mem_fun(*this, &PESelector::slotModuleExecuteBlock));
        signal->connect(sigc::mem_fun(*this, &PESelector::slotExecuteBlock));
    }
}

void PESelector::slotModuleExecuteBlock(S2EExecutionState *state, uint64_t pc)
{
    // set a flag for in-module blocks that will be checked in the generic
    // onExecute handler to distinguish between local and external blocks
    DECLARE_PLUGINSTATE(PESelectorState, state);
    plgState->pcInModule = true;
}

void PESelector::slotExecuteBlock(S2EExecutionState *state, uint64_t pc)
{
    // call onModuleReturn if a library call just returned
    // call onModuleExecute if a BB in the module is executed
    // call onModuleLeave on a library call
    DECLARE_PLUGINSTATE(PESelectorState, state);

    if (plgState->pcInModule) {
        if (plgState->inLibCall) {
            plgState->inLibCall = false;
            onModuleReturn.emit(state, pc);
        }
        onModuleExecute.emit(state, pc);
        plgState->prevPc = pc;
    }
    else if (plgState->prevPcInModule) {
        plgState->inLibCall = true;
        onModuleLeave.emit(state, pc);
    }

    plgState->prevPcInModule = plgState->pcInModule;
    plgState->pcInModule = false;
}

void PESelector::slotProcessUnload(S2EExecutionState *state, uint64_t pid) {
    if (m_moduleLoaded && pid == m_moduleDesc.Pid) {
        s2e()->getMessagesStream(state) << "process terminated, killing state\n";
        s2e()->getExecutor()->suspendState(state);
    }
}

uint64_t PESelector::getPrevLocalPc(S2EExecutionState *state)
{
    DECLARE_PLUGINSTATE(PESelectorState, state);
    return plgState->prevPc;
}

/*
 * Execution-state-specific plugin state
 */

PESelectorState::PESelectorState() :
    prevPcInModule(false), inLibCall(true), prevPc(0) {}

PESelectorState::~PESelectorState() {}

} // namespace plugins
} // namespace s2e
