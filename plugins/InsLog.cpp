#include <iostream>
#include <s2e/S2E.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>
#include "InsLog.h"

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(InsLog, "InsLog S2E plugin", "",);

void InsLog::initialize()
{
    m_of.open(s2e()->getOutputFilename("inslog.out").c_str(),
            std::ios::out | std::ios::trunc);

    s2e()->getCorePlugin()->onTranslateInstructionStart.connect(
            sigc::mem_fun(*this, &InsLog::onTranslateInstructionStart));
}

InsLog::~InsLog() {
    m_of.close();
}

void InsLog::onTranslateInstructionStart(ExecutionSignal *signal,
                                      S2EExecutionState *state,
                                      TranslationBlock *tb,
                                      uint64_t pc)
{
    m_of << "Translating instruction at " << std::hex << pc << std::dec << std::endl;
    signal->connect(sigc::mem_fun(*this, &InsLog::onExecuteInstructionStart));
}

void InsLog::onExecuteInstructionStart(S2EExecutionState *state, uint64_t pc)
{
    m_of << "Executing instruction at " << std::hex << pc << std::dec << std::endl;
}

} // namespace plugins
} // namespace s2e
