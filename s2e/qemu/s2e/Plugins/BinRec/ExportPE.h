#ifndef S2E_PLUGINS_EXPORTPE_H
#define S2E_PLUGINS_EXPORTPE_H

#include <vector>
#include <map>
#include <iostream>
#include <fstream>
#include <stdio.h>

#include <s2e/Plugin.h>
#include <s2e/S2EExecutionState.h>

#include <llvm/Module.h>

#include "Export.h"
#include "PELibCallMonitor.h"

namespace s2e {
namespace plugins {

class ExportPE : public Export
{
    S2E_PLUGIN
public:
    ExportPE(S2E *s2e) : Export(s2e) {}
    void initialize();

#ifdef TARGET_I386
    ~ExportPE();

    void slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module);
    void slotModuleExecute(S2EExecutionState *state, uint64_t pc);
    void slotLibCall(S2EExecutionState *state,
            const ModuleDescriptor &lib, const std::string &routine,
            uint64_t pc, PELibCallMonitor::ReturnSignal &onReturn);
    void slotStateFork(S2EExecutionState *state,
            const std::vector<S2EExecutionState*> &newStates,
            const std::vector<klee::ref<klee::Expr> > &newCondition);

private:
    ConfigFile::string_list m_baseDirs;
    ModuleSelector *m_selector;

    bool loadSections(S2EExecutionState *state);
#endif
};

class ExportPEState : public PluginState
{
    PLUGIN_STATE(ExportPEState);

    uint64_t prevPc;

    friend class ExportPE;
};

} // namespace plugins
} // namespace s2e

#endif // S2E_PLUGINS_EXPORTPE_H
