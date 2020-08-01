#ifndef __PLUGIN_ELFSELECTOR_H__
#define __PLUGIN_ELFSELECTOR_H__

#include <s2e/Plugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/ModuleExecutionDetector.h>

#include "ModuleSelector.h"
#include "util.h"

namespace s2e {
namespace plugins {

/**
 *  Base class for binary file code selectors
 */
class ELFSelector : public ModuleSelector
{
    S2E_PLUGIN
public:
    ELFSelector(S2E* s2e) : ModuleSelector(s2e) {}

    void initialize();

    virtual uint64_t getPrevLocalPc(S2EExecutionState *state);

private:
    void slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module);
    void slotTranslateBlockStart(ExecutionSignal*, S2EExecutionState *state,
            TranslationBlock *tb, uint64_t pc);
    void slotModuleTranslateBlockStart(ExecutionSignal*, S2EExecutionState *state,
            const ModuleDescriptor &module, TranslationBlock *tb, uint64_t pc);
    void slotExecuteBlock(S2EExecutionState *state, uint64_t pc);
    void slotModuleExecuteBlock(S2EExecutionState *state, uint64_t pc);

    ModuleDescriptor m_modInitEnv;

    bool m_moduleInitialized;
};

class ELFSelectorState : public PluginState
{
    PLUGIN_STATE(ELFSelectorState)

    bool pcInModule, prevPcInModule, inLibCall;
    uint64_t prevPc;

    friend class ELFSelector;
};

} // namespace plugins
} // namespace s2e

#endif // __PLUGIN_ELFSELECTOR_H__
