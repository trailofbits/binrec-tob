#include "ExportELF.h"
#include "ModuleSelector.h"
#include "util.h"
#include <s2e/S2E.h>
#include <cassert>
#include <llvm/Constants.h>
#include <llvm/Function.h>
#include <llvm/LLVMContext.h>
#include <llvm/Metadata.h>

using namespace llvm;

namespace s2e {
namespace plugins {

S2E_DEFINE_PLUGIN(ExportELF, "Exports LLVM bitcode for an ELF binary", "ExportELF", "ModuleSelector");

void ExportELF::initialize() {
    Export::initialize();

    m_baseDirs = s2e()->getConfig()->getStringList(getConfigKey() + ".baseDirs");

    m_exportInterval = s2e()->getConfig()->getInt(getConfigKey() + ".exportInterval", 0);

    ModuleSelector *selector = (ModuleSelector *)(s2e()->getPlugin("ModuleSelector"));
    selector->onModuleLoad.connect(sigc::mem_fun(*this, &ExportELF::slotModuleLoad));
    selector->onModuleExecute.connect(sigc::mem_fun(*this, &ExportELF::slotModuleExecute));

    s2e()->getCorePlugin()->onStateFork.connect(sigc::mem_fun(*this, &ExportELF::slotStateFork));

    s2e()->getDebugStream() << "[ExportELF] Plugin initialized\n";
}

void ExportELF::slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module) {
    initializeModule(module, m_baseDirs);
}

void ExportELF::slotModuleExecute(S2EExecutionState *state, uint64_t pc) {
    DECLARE_PLUGINSTATE(ExportELFState, state);

    if (plgState->doExport) {
        exportBB(state, pc);
        addSuccessor(plgState->prevPc, pc);
    }

    plgState->prevPc = pc;
}

void ExportELF::slotStateFork(S2EExecutionState *state, const std::vector<S2EExecutionState *> &newStates,
                              const std::vector<klee::ref<klee::Expr>> &newCondition) {
    stopRegeneratingBlocks();
}

ExportELFState::ExportELFState() : prevPc(0), doExport(true) {}
ExportELFState::~ExportELFState() {}

} // namespace plugins
} // namespace s2e
