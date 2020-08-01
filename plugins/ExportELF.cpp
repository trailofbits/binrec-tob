#include <cassert>

#include <s2e/S2E.h>
#include <s2e/Plugins/ModuleExecutionDetector.h>

#include <llvm/LLVMContext.h>
#include <llvm/Function.h>
#include <llvm/Metadata.h>
#include <llvm/Constants.h>

#include "ModuleSelector.h"
#include "ExportELF.h"
#include "util.h"

namespace s2e {
namespace plugins {

using namespace llvm;

S2E_DEFINE_PLUGIN(ExportELF,
                  "Exports LLVM bitcode for an ELF binary",
                  "ExportELF", "ModuleSelector");

#ifdef TARGET_I386

void ExportELF::initialize()
{
    m_baseDirs = s2e()->getConfig()->getStringList(getConfigKey() + ".baseDirs");

    m_exportInterval =
        s2e()->getConfig()->getInt(getConfigKey() + ".exportInterval", 0);

    ModuleSelector *selector =
        (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));
    selector->onModuleLoad.connect(
            sigc::mem_fun(*this, &ExportELF::slotModuleLoad));
    selector->onModuleExecute.connect(
            sigc::mem_fun(*this, &ExportELF::slotModuleExecute));

    ModuleExecutionDetector *modEx =
        (ModuleExecutionDetector*)(s2e()->getPlugin("ModuleExecutionDetector"));
    modEx->onModuleSignal.connect(
            sigc::mem_fun(*this, &ExportELF::slotModuleSignal));

    s2e()->getCorePlugin()->onStateFork.connect(
            sigc::mem_fun(*this, &ExportELF::slotStateFork));

    s2e()->getDebugStream() << "[ExportELF] Plugin initialized\n";
}

ExportELF::~ExportELF()
{
}

void ExportELF::slotModuleLoad(S2EExecutionState *state,
                               const ModuleDescriptor &module)
{
    //TODO: if (config flag that binary is from host){
        //symlink binary from host srcdir to outdir
        initializeModule(module, m_baseDirs);
    //}else {
        //copy binary from inside qemu to outdir
        //
    //}
    //
}

void ExportELF::slotModuleExecute(S2EExecutionState *state, uint64_t pc)
{
    DECLARE_PLUGINSTATE(ExportELFState, state);
    //s2e()->getDebugStream() << "execute " << hexval(pc) << "\n";

    if (plgState->doExport) {
        exportBB(state, pc);
        addSuccessor(plgState->prevPc, pc);
    }

    plgState->prevPc = pc;
}

void ExportELF::slotModuleSignal(S2EExecutionState *state, const ModuleExecutionCfg &desc)
{
    if (desc.moduleName == "init_env.so") {
        DECLARE_PLUGINSTATE(ExportELFState, state);
        s2e()->getMessagesStream(state) << "stopped exporting LLVM code\n";
        plgState->doExport = false;
    }
}

void ExportELF::slotStateFork(S2EExecutionState *state,
        const std::vector<S2EExecutionState*> &newStates,
        const std::vector<klee::ref<klee::Expr> > &newCondition)
{
    stopRegeneratingBlocks();
}

#else // TARGET_I386

void ExportELF::initialize()
{
    s2e()->getWarningsStream() << "[ExportELF] This plugin is only suited for i386\n";
}

#endif

/*
 * Execution-state-specific plugin state
 */

ExportELFState::ExportELFState() : prevPc(0), doExport(true) {}

ExportELFState::~ExportELFState() {}

} // namespace plugins
} // namespace s2e
