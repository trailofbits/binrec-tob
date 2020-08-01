#include <stdio.h>
#include <s2e/S2E.h>
#include <s2e/S2EExecutor.h>
#include <s2e/ConfigFile.h>
#include <s2e/Plugins/CorePlugin.h>
#include "ModuleSelector.h"
#include "AddrLog.h"

#define LOG_SEPARATOR 0
#define STATE_FORK    1
#define STATE_SWITCH  2

namespace s2e {
namespace plugins {

using std::cerr;

S2E_DEFINE_PLUGIN(AddrLog,
                  "Log addresses of executed basic blocks in log file",
                  "AddrLog", "ModuleSelector");

void AddrLog::initialize()
{
    m_logAll = s2e()->getConfig()->getBool(
            getConfigKey() + ".logAll", false);
    std::string logFileName = s2e()->getConfig()->getString(
            getConfigKey() + ".logFile", "addrlog.dat");

    m_logFile = fopen(s2e()->getOutputFilename(logFileName).c_str(), "w");

    s2e()->getCorePlugin()->onStateFork.connect(
            sigc::mem_fun(*this, &AddrLog::slotStateFork));
    s2e()->getCorePlugin()->onStateSwitch.connect(
            sigc::mem_fun(*this, &AddrLog::slotStateSwitch));

    if (m_logAll) {
        s2e()->getCorePlugin()->onTranslateBlockStart.connect(
                sigc::mem_fun(*this, &AddrLog::slotTranslateBlock));
    } else {
        ModuleSelector *selector =
            (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));
        selector->onModuleExecute.connect(
                sigc::mem_fun(*this, &AddrLog::slotExecuteBlock));
    }

    s2e()->getMessagesStream() << "[AddrLog] plugin initialized\n";
}

AddrLog::~AddrLog() {
    fclose(m_logFile);
}

void AddrLog::slotStateFork(S2EExecutionState *state,
        const std::vector<S2EExecutionState*> &newStates,
        const std::vector<klee::ref<klee::Expr> > &newCondition)
{
    foreach2(newState, newStates.begin(), newStates.end())
        logAddr(STATE_FORK, (*newState)->getID());
}

void AddrLog::slotStateSwitch(S2EExecutionState *state,
                                S2EExecutionState *newState)
{
    logAddr(STATE_SWITCH, newState->getID());
}

void AddrLog::slotTranslateBlock(ExecutionSignal *signal,
    S2EExecutionState *state, TranslationBlock *tb, uint64_t pc)
{
    signal->connect(sigc::mem_fun(*this, &AddrLog::slotExecuteBlock));
}

void AddrLog::slotExecuteBlock(S2EExecutionState *state, uint64_t pc)
{
    unsigned lastPc = state->getTb()->llvm_last_pc_in_bb;
    assert(lastPc && "last PC not set");
    logAddr(pc, lastPc);
}

void AddrLog::logAddr(uint32_t a, uint32_t b)
{
    fwrite(&a, sizeof (uint32_t), 1, m_logFile);
    fwrite(&b, sizeof (uint32_t), 1, m_logFile);
    fflush(m_logFile);
}

} // namespace plugins
} // namespace s2e
