#ifndef __PLUGIN_REGISTERLOG_H__
#define __PLUGIN_REGISTERLOG_H__

#include <fstream>

#include <s2e/Plugin.h>
#include <s2e/S2EExecutionState.h>

namespace s2e {
namespace plugins {

using namespace llvm;

/**
 *  Base class for binary file code selectors
 */
class RegisterLog : public Plugin
{
    S2E_PLUGIN
public:
    RegisterLog(S2E* s2e): Plugin(s2e) {}
    ~RegisterLog();

    void initialize();
    void slotStateFork(S2EExecutionState *state,
            const std::vector<S2EExecutionState*> &newStates,
            const std::vector<klee::ref<klee::Expr> > &newCondition);
private:
    std::ofstream m_logFile;
    sigc::connection m_connExec, m_connFork;

    void writeLogLine(S2EExecutionState *state, uint64_t pc);
};

} // namespace plugins
} // namespace s2e

#endif // __PLUGIN_REGISTERLOG_H__
