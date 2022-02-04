#ifndef __PLUGIN_MODULESELECTOR_H__
#define __PLUGIN_MODULESELECTOR_H__

#include <cassert>
#include <s2e/Plugin.h>
#include <s2e/Plugins/OSMonitors/ModuleDescriptor.h>
#include <s2e/S2EExecutionState.h>

namespace s2e {
    namespace plugins {

        /**
         *  Base class for binary file code selectors
         */
        class ModuleSelector : public Plugin {
        protected:
            ModuleSelector(S2E *s2e) : Plugin(s2e) {}

        public:
            sigc::signal<void, S2EExecutionState *, const ModuleDescriptor &> onModuleLoad;
            sigc::signal<void, S2EExecutionState *, uint64_t> onModuleExecute;
            sigc::signal<void, S2EExecutionState *, uint64_t> onModuleLeave;
            sigc::signal<void, S2EExecutionState *, uint64_t> onModuleReturn;

            virtual uint64_t getPrevLocalPc(S2EExecutionState *state)
            {
                // dummy implementation to make the linker happy
                assert(false);
                return 0;
            }
        };

    } // namespace plugins
} // namespace s2e

#endif // __PLUGIN_MODULESELECTOR_H__
