#ifndef __PLUGIN_SYMBOLIZEARGSPE_H__
#define __PLUGIN_SYMBOLIZEARGSPE_H__

#include <fstream>

#include <s2e/Plugin.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/ModuleDescriptor.h>

#include "PELibCallMonitor.h"

namespace s2e {
namespace plugins {

using namespace llvm;

/**
 *  Base class for binary file code selectors
 */
class SymbolizeArgsPE : public Plugin
{
    S2E_PLUGIN
public:
    SymbolizeArgsPE(S2E* s2e) : Plugin(s2e) {}
    ~SymbolizeArgsPE() {}

    void initialize();

private:
    bool m_symbolizeArgv0;

    void slotLibCall(S2EExecutionState *state,
            const ModuleDescriptor &lib, const std::string &routine,
            uint64_t pc, PELibCallMonitor::ReturnSignal &onReturn);
    void slotGetMainArgsReturn(S2EExecutionState *state, uint64_t pc);

    void makeSymbolic(S2EExecutionState *state, uint32_t addr, size_t size,
            const std::string &label, bool makeConcolic);
};

class SymbolizeArgsPEState : public PluginState
{
    PLUGIN_STATE(SymbolizeArgsPEState)

    uint32_t argcAddr, argvAddr;

    friend class SymbolizeArgsPE;
};

} // namespace plugins
} // namespace s2e

#endif // __PLUGIN_SYMBOLIZEARGSPE_H__
