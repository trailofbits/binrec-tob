#ifndef S2E_PLUGINS_ADDRLOG_H
#define S2E_PLUGINS_ADDRLOG_H

#include <s2e/Plugin.h>
#include <s2e/S2EExecutionState.h>
#include <stdio.h>
#include <vector>

namespace s2e {
    namespace plugins {

        class AddrLog : public Plugin {
            S2E_PLUGIN
        public:
            AddrLog(S2E *s2e) : Plugin(s2e) {}

            void initialize();

            ~AddrLog();
            void slotStateFork(
                S2EExecutionState *state,
                const std::vector<S2EExecutionState *> &newStates,
                const std::vector<klee::ref<klee::Expr>> &newCondition);
            void slotStateSwitch(S2EExecutionState *state, S2EExecutionState *newState);

            void slotTranslateBlock(
                ExecutionSignal *signal,
                S2EExecutionState *state,
                TranslationBlock *tb,
                uint64_t pc);
            void slotExecuteBlock(S2EExecutionState *state, uint64_t pc);

        private:
            bool m_logAll;
            FILE *m_logFile;

            void logAddr(uint32_t a, uint32_t b);
        };

    } // namespace plugins
} // namespace s2e

#endif // S2E_PLUGINS_ADDRLOG
