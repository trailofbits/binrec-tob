#ifndef S2E_PLUGINS_EXPORTELF_H
#define S2E_PLUGINS_EXPORTELF_H

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include <s2e/Plugin.h>
#include <s2e/ConfigFile.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>

#include "Export.h"
#include "util.h"

namespace s2e {
namespace plugins {

class ExportELF : public Export
{
    S2E_PLUGIN
public:
    ExportELF(S2E *s2e) : Export(s2e) {}
    void initialize();

#ifdef TARGET_I386
    ~ExportELF();

    void slotModuleLoad(S2EExecutionState *state,
            const ModuleDescriptor &module);
    void slotModuleExecute(S2EExecutionState* state, uint64_t pc);
    void slotModuleSignal(S2EExecutionState *state,
            const ModuleExecutionCfg &desc);
    void slotStateFork(S2EExecutionState *state,
            const std::vector<S2EExecutionState*> &newStates,
            const std::vector<klee::ref<klee::Expr> > &newCondition);

private:
    ConfigFile::string_list m_baseDirs;
#endif
};

class ExportELFState : public PluginState
{
    PLUGIN_STATE(ExportELFState)

    uint64_t prevPc;
    bool doExport;

    friend class ExportELF;
};

} // namespace plugins
} // namespace s2e

#endif // S2E_PLUGINS_EXPORTELF_H
