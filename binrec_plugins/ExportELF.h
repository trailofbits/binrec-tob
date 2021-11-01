#ifndef S2E_PLUGINS_EXPORTELF_H
#define S2E_PLUGINS_EXPORTELF_H

#include "Export.h"
#include "util.h"
#include <s2e/ConfigFile.h>
#include <s2e/Plugin.h>
#include <s2e/CorePlugin.h>
#include <s2e/Plugins/OSMonitors/Support/ModuleExecutionDetector.h>
#include <s2e/S2EExecutionState.h>
#include <fstream>
#include <iostream>
#include <map>
#include <stdio.h>
#include <vector>

namespace s2e {
namespace plugins {

class ExportELF : public Export {
    S2E_PLUGIN
public:
    ExportELF(S2E *s2e) : Export(s2e) {}

    void initialize();

    void slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module);
    void slotModuleExecute(S2EExecutionState *state, uint64_t pc);
    void slotStateFork(S2EExecutionState *state, const std::vector<S2EExecutionState *> &newStates,
                       const std::vector<klee::ref<klee::Expr>> &newCondition);

private:
    ConfigFile::string_list m_baseDirs;
};

class ExportELFState : public PluginState {
    PLUGIN_STATE(ExportELFState)

    uint64_t prevPc;
    bool doExport;

    friend class ExportELF;
};

} // namespace plugins
} // namespace s2e

#endif
