#ifndef __PLUGIN_PELIBCALLMONITOR_H__
#define __PLUGIN_PELIBCALLMONITOR_H__

#include "ModuleSelector.h"
#include "util.h"
#include <map>
#include <s2e/ConfigFile.h>
#include <s2e/Plugin.h>
#include <s2e/Plugins/OSMonitors/ModuleDescriptor.h>
#include <s2e/Plugins/OSMonitors/Support/ModuleExecutionDetector.h>
#include <s2e/S2EExecutionState.h>
#include <vector>

namespace s2e {
    namespace plugins {

        /**
         *  Base class for binary file code selectors
         */
        class PELibCallMonitor : public ModuleSelector {
            S2E_PLUGIN
        public:
            typedef sigc::signal<void, S2EExecutionState *, uint64_t> ReturnSignal;
            typedef std::pair<const ModuleDescriptor *, std::string> Symbol;

            PELibCallMonitor(S2E *s2e) : ModuleSelector(s2e) {}
            void initialize();

            sigc::signal<
                void,
                S2EExecutionState *,
                const ModuleDescriptor &,
                const std::string &,
                uint64_t,
                ReturnSignal &>
                onLibCall;

#ifdef TARGET_I386
        private:
            ModuleSelector *m_selector;

            std::map<const std::string, ModuleDescriptor> m_loadedModules;
            ModuleDescriptor m_moduleDesc;
            std::map<uint64_t, Symbol> m_symbols;
            bool m_mainModuleLoaded;

            void slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module);
            void slotMainModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module);
            void slotModuleLeave(S2EExecutionState *state, uint64_t pc);
            void slotModuleReturn(S2EExecutionState *state, uint64_t pc);

            bool reloadImports(S2EExecutionState *state);
#endif
        };

        class PELibCallMonitorState : public PluginState {
            PLUGIN_STATE(PELibCallMonitorState)

            uint64_t callPc;
            PELibCallMonitor::ReturnSignal onLibReturn;

            friend class PELibCallMonitor;
        };

    } // namespace plugins
} // namespace s2e

#endif // __PLUGIN_PELIBCALLMONITOR_H__
