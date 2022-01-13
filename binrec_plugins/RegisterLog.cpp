#include <cassert>

#include <s2e/cpu.h>
#include <s2e/S2E.h>
#include <s2e/Utils.h>
#include <s2e/ConfigFile.h>
#include <s2e/CorePlugin.h>

#include "RegisterLog.h"
#include "ModuleSelector.h"

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(RegisterLog,
                  "Log register values at basic block start (don't use with symbolic execution)",
                  "RegisterLog", "ModuleSelector");

void RegisterLog::initialize()
{
    std::string logFileName = s2e()->getConfig()->getString(
            getConfigKey() + ".logFile", "register-log.txt");
    m_logFile.open(s2e()->getOutputFilename(logFileName).c_str());
    assert(m_logFile);

    ModuleSelector *selector =
        (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));
    m_connExec = selector->onModuleExecute.connect(
            sigc::mem_fun(*this, &RegisterLog::writeLogLine));
    m_connFork = s2e()->getCorePlugin()->onStateFork.connect(
            sigc::mem_fun(*this, &RegisterLog::slotStateFork));
}

RegisterLog::~RegisterLog()
{
    m_logFile.close();
}


void RegisterLog::writeLogLine(S2EExecutionState *state, uint64_t pc)
{
    assert(state->regs()->getPc() == pc);

#define PRT_VAL(prefix, value) \
    m_logFile << prefix "=" << std::setw(8) << hexval(value);

    auto cpuState = state->regs()->getCpuState();
#ifdef TARGET_I386
    m_logFile << std::left;
    PRT_VAL("pc",      pc);
    PRT_VAL(" eax",    cpuState->regs[R_EAX]);
    PRT_VAL(" ebx",    cpuState->regs[R_EBX]);
    PRT_VAL(" ecx",    cpuState->regs[R_ECX]);
    PRT_VAL(" edx",    cpuState->regs[R_EDX]);
    PRT_VAL(" ebp",    cpuState->regs[R_EBP]);
    PRT_VAL(" esp",    cpuState->regs[R_ESP]);
    PRT_VAL(" esi",    cpuState->regs[R_ESI]);
    PRT_VAL(" edi",    cpuState->regs[R_EDI]);
    PRT_VAL(" cc_src", cpuState->cc_src);
    PRT_VAL(" cc_dst", cpuState->cc_dst);
    PRT_VAL(" cc_op",  cpuState->cc_op);
    PRT_VAL(" df",     cpuState->df);
    PRT_VAL(" mflags", cpuState->mflags);
    m_logFile << '\n';
#endif

#undef PRT_VAL
}

void RegisterLog::slotStateFork(S2EExecutionState *state,
        const std::vector<S2EExecutionState*> &newStates,
        const std::vector<klee::ref<klee::Expr> > &newCondition)
{
    // don't log if symbolic execution is enabled to prevent concretizing
    s2e()->getDebugStream(state) << "[RegisterLog] stopping register "
        "logging because symbolic execution is enabled\n";
    m_logFile << "stopped logging because of state fork\n";
    m_connExec.disconnect();
    m_connFork.disconnect();
}

} // namespace plugins
} // namespace s2e
