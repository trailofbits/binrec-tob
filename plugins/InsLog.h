#ifndef S2E_PLUGINS_INSLOG_H
#define S2E_PLUGINS_INSLOG_H

#include <fstream>
#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

namespace s2e {
namespace plugins {

class InsLog : public Plugin
{
    S2E_PLUGIN
public:
    InsLog(S2E* s2e): Plugin(s2e) {}
    ~InsLog();

    void initialize();
    void onTranslateInstructionStart(ExecutionSignal*, S2EExecutionState *state,
        TranslationBlock *tb, uint64_t pc);
    void onExecuteInstructionStart(S2EExecutionState* state, uint64_t pc);
private:
    std::ofstream m_of;
};

} // namespace plugins
} // namespace s2e

#endif // S2E_PLUGINS_INSLOG_H
