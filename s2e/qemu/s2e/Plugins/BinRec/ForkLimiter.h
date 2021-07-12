#ifndef S2E_PLUGINS_ForkLimiter_H
#define S2E_PLUGINS_ForkLimiter_H

#include <llvm/ADT/DenseMap.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/Plugin.h>
#include <s2e/Plugins/ModuleExecutionDetector.h>
#include <s2e/S2EExecutionState.h>

#include "util.h"


namespace s2e {
namespace plugins {

class ForkLimiter : public Plugin {
    S2E_PLUGIN
public:
    ForkLimiter(S2E *s2e) : Plugin(s2e) {
    }

    void initialize();

private:
    typedef llvm::DenseMap<uint64_t, uint64_t> ForkCounts;
    typedef std::map<std::string, ForkCounts> ModuleForkCounts;

    ModuleExecutionDetector *m_detector;
    ModuleForkCounts m_forkCount;
//   std::unordered_map<uint64_t, uint64_t> stateRunTimes;

    unsigned m_timeLimit;
    unsigned m_forkLimit;
//    unsigned m_processForkDelay;

    ModuleDescriptor m_binModule;

    unsigned m_timerTicks;
//    S2EExecutionState* m_currentState;

    void onTimer();
    void onStateSwitch(S2EExecutionState *currentState, S2EExecutionState *nextState);
    void slotModuleLoad(S2EExecutionState *state,
                                 const ModuleDescriptor &module);


    void slotModuleLeave(S2EExecutionState *state, uint64_t pc);
    void slotModuleReturn(S2EExecutionState *state, uint64_t pc);
    
    void onSlotTranslateBlockStart(
            ExecutionSignal *signal,
            S2EExecutionState* state,
            TranslationBlock *tb,
            uint64_t pc);

    void onTermination1(S2EExecutionState* state, uint64_t sourcePc);
    void onTermination2(S2EExecutionState* state, uint64_t sourcePc);
    void onTermination3(S2EExecutionState* state, uint64_t sourcePc);

    void onFork(S2EExecutionState *state, const std::vector<S2EExecutionState *> &newStates,
                const std::vector<klee::ref<klee::Expr> > &newConditions);
};

enum Reason {forkLimit, timeout, libFork, noReason};

class ForkLimiterState : public PluginState
{
    PLUGIN_STATE(ForkLimiterState)

    unsigned timerTicks;
    bool terminate;
    Reason reason;

    friend class ForkLimiter;
};

} // namespace plugins
} // namespace s2e

#endif // S2E_PLUGINS_ForkLimiter_H

