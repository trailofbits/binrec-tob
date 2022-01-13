#include <cassert>

#include <s2e/S2E.h>
#include <s2e/Utils.h>
#include <s2e/ConfigFile.h>

#include "SymbolizeArgsPE.h"

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(SymbolizeArgsPE,
                  "Generate symbolic command-line arguments for a PE binary",
                  "SymbolizeArgsPE", "PELibCallMonitor");

void SymbolizeArgsPE::initialize()
{
    m_symbolizeArgv0 = s2e()->getConfig()->getBool(getConfigKey() + ".symbolizeArgv0", true);

    PELibCallMonitor *sym = (PELibCallMonitor*)(s2e()->getPlugin("PELibCallMonitor"));
    sym->onLibCall.connect(sigc::mem_fun(*this, &SymbolizeArgsPE::slotLibCall));
}

template<class T>
static T readMemVal(S2EExecutionState *state, uint64_t addr)
{
    T val;
    assert(state->readMemoryConcrete(addr, &val, sizeof (T)));
    return val;
}

void SymbolizeArgsPE::slotLibCall(S2EExecutionState *state,
        const ModuleDescriptor &lib, const std::string &routine,
        uint64_t pc, PELibCallMonitor::ReturnSignal &onReturn)
{
    if (lib.Name == "msvcrt.dll" && routine == "__getmainargs") {
        DECLARE_PLUGINSTATE(SymbolizeArgsPEState, state);
        plgState->argcAddr = readMemVal<uint32_t>(state, state->getSp() + sizeof(uint32_t));
        plgState->argvAddr = readMemVal<uint32_t>(state, state->getSp() + 2 * sizeof(uint32_t));
        onReturn.connect(sigc::mem_fun(*this, &SymbolizeArgsPE::slotGetMainArgsReturn));
    }
}

void SymbolizeArgsPE::slotGetMainArgsReturn(S2EExecutionState *state, uint64_t pc)
{
    DECLARE_PLUGINSTATE(SymbolizeArgsPEState, state);

    int32_t argc = readMemVal<int32_t>(state, plgState->argcAddr);
    uint32_t argv = readMemVal<uint32_t>(state, plgState->argvAddr);

    s2e()->getDebugStream() << "returned from __getmainargs\n";
    s2e()->getDebugStream() << "  argc: " << argc << '\n';

    for (int32_t i = 0; i < argc; i++) {
        std::string arg;
        uint32_t argAddr = readMemVal<uint32_t>(state, argv + i * sizeof(int32_t));
        assert(state->readString(argAddr, arg));
        s2e()->getDebugStream() << "  argv[" << i << "]: \"" << arg << "\"\n";

        if (m_symbolizeArgv0 || i != 0)
            makeSymbolic(state, argAddr, arg.size(), "argv[" + to_string(i) + "]", true);
    }
}

void SymbolizeArgsPE::makeSymbolic(S2EExecutionState *state, uint32_t addr,
        size_t size, const std::string &label, bool makeConcolic)
{
    std::vector<klee::ref<klee::Expr> > symb;

    if (!state->isSymbolicExecutionEnabled())
        state->enableSymbolicExecution();

    s2e()->getInfoStream(state)
        << "inserting symbolic data at " << hexval(addr)
        << " of size " << size << " with label \"" << label << "\"\n";

    if (makeConcolic) {
        std::vector<uint8_t> concreteData;
        for (size_t i = 0; i < size; ++i) {
            concreteData.push_back(readMemVal<uint8_t>(state, addr + i));
        }
        symb = state->createConcolicArray(label, size, concreteData);
    } else {
        symb = state->createSymbolicArray(label, size);
    }

    for (size_t i = 0; i < size; i++) {
        if (!state->writeMemory8(addr + i, symb[i])) {
            s2e()->getWarningsStream(state)
                << "can not insert symbolic value at " << hexval(addr + i)
                << ": can not write to memory\n";
        }
    }
}

SymbolizeArgsPEState::SymbolizeArgsPEState() {}

SymbolizeArgsPEState::~SymbolizeArgsPEState() {}

} // namespace plugins
} // namespace s2e
