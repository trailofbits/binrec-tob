#include <s2e/S2E.h>
#include <s2e/S2EExecutor.h>
#include <s2e/Plugins/OSMonitors/Support/ModuleExecutionDetector.h>
#include <s2e/Plugins/WindowsInterceptor/WindowsImage.h>

#include "PELibCallMonitor.h"
#include "ModuleSelector.h"

namespace s2e {
namespace plugins {

typedef std::map<uint64_t, PELibCallMonitor::Symbol>::iterator SymbolIterator;

using namespace llvm;

S2E_DEFINE_PLUGIN(PELibCallMonitor,
                  "Emit events for calls and returns of library routines",
                  "PELibCallMonitor", "ModuleSelector", "ModuleExecutionDetector");

#ifdef TARGET_I386

void PELibCallMonitor::initialize()
{
    m_mainModuleLoaded = false;

    ModuleExecutionDetector *modEx =
        (ModuleExecutionDetector*)(s2e()->getPlugin("ModuleExecutionDetector"));
    m_selector = (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));

    modEx->onModuleLoad.connect(
            sigc::mem_fun(*this, &PELibCallMonitor::slotModuleLoad));
    m_selector->onModuleLoad.connect(
            sigc::mem_fun(*this, &PELibCallMonitor::slotMainModuleLoad));
    m_selector->onModuleLeave.connect(
            sigc::mem_fun(*this, &PELibCallMonitor::slotModuleLeave));
    m_selector->onModuleReturn.connect(
            sigc::mem_fun(*this, &PELibCallMonitor::slotModuleReturn));

    s2e()->getInfoStream() << "[PELibCallMonitor] Plugin initialized\n";
}

void PELibCallMonitor::slotModuleLoad(S2EExecutionState *state,
                               const ModuleDescriptor &module)
{
    if (m_mainModuleLoaded && module.Pid == m_moduleDesc.Pid) {
        s2e()->getDebugStream() << "[PELibCallMonitor] module '" << module.Name << "' loaded\n";

        assert(m_loadedModules.find(module.Name) == m_loadedModules.end());
        m_loadedModules[module.Name] = module;

        if (m_mainModuleLoaded) {
            if (reloadImports(state))
                s2e()->getDebugStream() << "[PELibCallMonitor] successfully reloaded imports\n";
            else
                s2e()->getDebugStream() << "[PELibCallMonitor] error reloading imports\n";
        }
    }
}

void PELibCallMonitor::slotMainModuleLoad(S2EExecutionState *state,
                               const ModuleDescriptor &module)
{
    m_mainModuleLoaded = true;
    m_moduleDesc = module;
}

void PELibCallMonitor::slotModuleLeave(S2EExecutionState *state, uint64_t pc)
{
    DECLARE_PLUGINSTATE(PELibCallMonitorState, state);

    const SymbolIterator it = m_symbols.find(pc);

    if (it != m_symbols.end()) {
        const Symbol &sym = it->second;
        plgState->callPc = pc;
        onLibCall.emit(state, *sym.first, sym.second,
                m_selector->getPrevLocalPc(state), plgState->onLibReturn);
    } else {
        plgState->callPc = 0;

        s2e()->getDebugStream() << "could not find symbol for " << hexval(pc);

        foreach2(it, m_loadedModules.begin(), m_loadedModules.end()) {
            const ModuleDescriptor &mod = it->second;

            if (mod.Contains(pc)) {
                s2e()->getDebugStream() << " (should be in " << mod.Name << ")";
                break;
            }
        }

        s2e()->getDebugStream() << "\n";
    }
}

void PELibCallMonitor::slotModuleReturn(S2EExecutionState *state, uint64_t pc)
{
    DECLARE_PLUGINSTATE(PELibCallMonitorState, state);

    //if (!plgState->onLibReturn.empty()) {
    if (plgState->callPc) {
        plgState->onLibReturn.emit(state, pc);
        plgState->onLibReturn.disconnectAll();
        // we need to manually set m_size to 0 because otherwise
        // PELibCallMonitorState.clone() will fail on a state fork
        // TODO: make this into a pull request
        plgState->onLibReturn.m_size = 0;
    }
}

bool PELibCallMonitor::reloadImports(S2EExecutionState *state)
{
    WindowsImage img(state, m_moduleDesc.LoadBase);
    Imports imports = img.GetImports(state);

    foreach2(it, imports.begin(), imports.end()) {
        const std::string &libraryName = it->first;
        const ImportedFunctions &functions = it->second;

        if (m_loadedModules.find(libraryName) == m_loadedModules.end()) {
            assert(functions.size() == 0);
            continue;
        }

        const ModuleDescriptor *library = &m_loadedModules[libraryName];

        foreach2(import, functions.begin(), functions.end()) {
            const std::string &functionName = import->first;
            const uint64_t &address = import->second;

            const SymbolIterator symit = m_symbols.find(address);

            if (symit == m_symbols.end()) {
                m_symbols.insert(std::make_pair(address, std::make_pair(library, functionName)));

                s2e()->getDebugStream() << "loaded symbol at " << hexval(address)
                    << ": " << libraryName << "::" << functionName << '\n';
            } else {
                const Symbol &sym = symit->second;
                assert(sym.first->Name == library->Name);
                assert(sym.second == functionName);
            }
        }
    }

    return true;
}

#else

void PELibCallMonitor::initialize()
{
    s2e()->getInfoStream() << "[PELibCallMonitor] This plugin is only suited for i386\n";
}

#endif

PELibCallMonitorState::PELibCallMonitorState() {}

PELibCallMonitorState::~PELibCallMonitorState() {}

} // namespace plugins
} // namespace s2e
