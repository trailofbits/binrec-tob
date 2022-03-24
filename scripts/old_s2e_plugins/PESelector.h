#ifndef __PLUGIN_PESELECTOR_H__
#define __PLUGIN_PESELECTOR_H__

#include "ModuleSelector.h"
#include "util.h"
#include <s2e/ConfigFile.h>
#include <s2e/Plugin.h>
#include <s2e/Plugins/OSMonitors/ModuleDescriptor.h>
#include <s2e/S2EExecutionState.h>

namespace s2e {
    namespace plugins {

        /**
         *  Base class for binary file code selectors
         */
        class PESelector : public ModuleSelector {
            S2E_PLUGIN
        public:
            PESelector(S2E *s2e) : ModuleSelector(s2e) {}

            void initialize();

            virtual uint64_t getPrevLocalPc(S2EExecutionState *state);

        private:
            ConfigFile::string_list m_moduleNames;
            bool m_moduleLoaded;
            ModuleDescriptor m_moduleDesc;

            void slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module);
            void slotTranslateBlockStart(
                ExecutionSignal *signal,
                S2EExecutionState *state,
                TranslationBlock *tb,
                uint64_t pc);
            void slotModuleTranslateBlockStart(
                ExecutionSignal *,
                S2EExecutionState *state,
                const ModuleDescriptor &module,
                TranslationBlock *tb,
                uint64_t pc);
            void slotExecuteBlock(S2EExecutionState *state, uint64_t pc);
            void slotModuleExecuteBlock(S2EExecutionState *state, uint64_t pc);
            void slotProcessUnload(S2EExecutionState *state, uint64_t pid);

            bool isInterestingModule(const ModuleDescriptor &module);
        };

        class PESelectorState : public PluginState {
            PLUGIN_STATE(PESelectorState)

            bool pcInModule, prevPcInModule, inLibCall;
            uint64_t prevPc;

            friend class PESelector;
        };

    } // namespace plugins
} // namespace s2e

#endif // __PLUGIN_PESELECTOR_H__
