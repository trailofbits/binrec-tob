#include <cassert>

#include <s2e/S2E.h>
#include <s2e/Utils.h>
#include <s2e/ConfigFile.h>
#include <s2e/Plugins/CorePlugin.h>

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

#ifdef TARGET_I386
static uint32_t cpuVal(S2EExecutionState *state, unsigned offset)
{
    uint32_t value;
    state->readRegisterConcrete(state->getConcreteCpuState(),
            offset, (uint8_t*)&value, sizeof(value));
    return value;
}
#endif

void RegisterLog::writeLogLine(S2EExecutionState *state, uint64_t pc)
{
    assert(state->getPc() == pc);

#define PRT_VAL(prefix, value) \
    m_logFile << prefix "=" << std::setw(8) << hexval(value);

#ifdef TARGET_I386
    m_logFile << std::left;
    PRT_VAL("pc",      pc);
    PRT_VAL(" eax",    cpuVal(state, CPU_REG_OFFSET(R_EAX)));
    PRT_VAL(" ebx",    cpuVal(state, CPU_REG_OFFSET(R_EBX)));
    PRT_VAL(" ecx",    cpuVal(state, CPU_REG_OFFSET(R_ECX)));
    PRT_VAL(" edx",    cpuVal(state, CPU_REG_OFFSET(R_EDX)));
    PRT_VAL(" ebp",    cpuVal(state, CPU_REG_OFFSET(R_EBP)));
    PRT_VAL(" esp",    cpuVal(state, CPU_REG_OFFSET(R_ESP)));
    PRT_VAL(" esi",    cpuVal(state, CPU_REG_OFFSET(R_ESI)));
    PRT_VAL(" edi",    cpuVal(state, CPU_REG_OFFSET(R_EDI)));
    PRT_VAL(" cc_src", cpuVal(state, CPU_OFFSET(cc_src)));
    PRT_VAL(" cc_dst", cpuVal(state, CPU_OFFSET(cc_dst)));
    PRT_VAL(" cc_op",  cpuVal(state, CPU_OFFSET(cc_op)));
    PRT_VAL(" df",     state->readCpuState(CPU_OFFSET(df), 32));
    PRT_VAL(" mflags", state->readCpuState(CPU_OFFSET(mflags), 32));
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
