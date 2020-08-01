#include <s2e/S2E.h>
#include <s2e/S2EExecutor.h>

#include "ELFSelector.h"

namespace s2e {
namespace plugins {

using namespace llvm;

S2E_DEFINE_PLUGIN(ELFSelector,
                  "Emits execution events only for blocks in an ELF binary",
                  "ModuleSelector", "ModuleExecutionDetector");

void ELFSelector::initialize()
{
    m_moduleInitialized = false;

    ModuleExecutionDetector *modEx =
        (ModuleExecutionDetector*)(s2e()->getPlugin("ModuleExecutionDetector"));
    assert(modEx);

    modEx->onModuleLoad.connect(
            sigc::mem_fun(*this, &ELFSelector::slotModuleLoad));
    modEx->onModuleTranslateBlockStart.connect(
            sigc::mem_fun(*this, &ELFSelector::slotModuleTranslateBlockStart));
    s2e()->getCorePlugin()->onTranslateBlockStart.connect(
            sigc::mem_fun(*this, &ELFSelector::slotTranslateBlockStart));

    s2e()->getMessagesStream() << "[ELFSelector] Plugin initialized\n";
}

void ELFSelector::slotModuleLoad(S2EExecutionState *state,
                                 const ModuleDescriptor &module)
{
    if (module.Name == "init_env.so") {
        m_modInitEnv = module;
    } else {
        m_moduleInitialized = true;
        onModuleLoad.emit(state, module);
    }
}

void ELFSelector::slotTranslateBlockStart(
    ExecutionSignal *signal,
    S2EExecutionState *state,
    TranslationBlock *tb,
    uint64_t pc)
{
    if (m_moduleInitialized)
        signal->connect(sigc::mem_fun(*this, &ELFSelector::slotExecuteBlock));
}

void ELFSelector::slotModuleTranslateBlockStart(
    ExecutionSignal *signal,
    S2EExecutionState *state,
    const ModuleDescriptor &module,
    TranslationBlock *tb,
    uint64_t pc)
{
    // mark init_env.so blocks as external
    if (m_moduleInitialized && !m_modInitEnv.Contains(pc))
        signal->connect(sigc::mem_fun(*this, &ELFSelector::slotModuleExecuteBlock));
}

void ELFSelector::slotModuleExecuteBlock(S2EExecutionState *state, uint64_t pc)
{
    // set a flag for in-module blocks that will be checked in the generic
    // onExecute handler to distinguish between local and external blocks
    DECLARE_PLUGINSTATE(ELFSelectorState, state);
    plgState->pcInModule = true;
}

void ELFSelector::slotExecuteBlock(S2EExecutionState *state, uint64_t pc)
{
    // call onModuleReturn if a library call just returned
    // call onModuleExecute if a BB in the module is executed
    // call onModuleLeave on a library call
    DECLARE_PLUGINSTATE(ELFSelectorState, state);

    if (plgState->pcInModule) {
        if (plgState->inLibCall) {
            plgState->inLibCall = false;
            onModuleReturn.emit(state, pc);
        }
        onModuleExecute.emit(state, pc);
        plgState->prevPc = pc;
    }
    else if (plgState->prevPcInModule) {
        // ignore library calls to init_env.so
        if (m_modInitEnv.Contains(pc)) {
            s2e()->getDebugStream() << "ignoring init_env pc " << hexval(pc) << "\n";
        }
        // FIXME: this address keeps showing up...
        else if (pc == 0xc12c62ac) {
            s2e()->getDebugStream() << "ignoring weird pc " << hexval(pc) << "\n";
        }
        else {
            plgState->inLibCall = true;
            onModuleLeave.emit(state, pc);
        }
    }

    plgState->prevPcInModule = plgState->pcInModule;
    plgState->pcInModule = false;
}

uint64_t ELFSelector::getPrevLocalPc(S2EExecutionState *state)
{
    DECLARE_PLUGINSTATE(ELFSelectorState, state);
    return plgState->prevPc;
}

/*
 * Execution-state-specific plugin state
 */

ELFSelectorState::ELFSelectorState() :
    pcInModule(false), prevPcInModule(false), inLibCall(true), prevPc(0) {}

ELFSelectorState::~ELFSelectorState() {}

} // namespace plugins
} // namespace s2e
