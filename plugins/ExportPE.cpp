#include <limits.h>
#include <stdlib.h>
#include <cassert>

#include <s2e/S2E.h>
#include <s2e/S2EExecutor.h>
#include <s2e/ConfigFile.h>
#include <s2e/Utils.h>
#include <s2e/Plugins/WindowsInterceptor/WindowsImage.h>

#include <llvm/Function.h>
#include <llvm/Metadata.h>

#include "ExportPE.h"
#include "ModuleSelector.h"
#include "PELibCallMonitor.h"
#include "util.h"

namespace s2e {
namespace plugins {

using namespace llvm;

S2E_DEFINE_PLUGIN(ExportPE,
                  "Log addresses of executed basic blocks",
                  "ExportPE", "ModuleSelector", "PELibCallMonitor");

#ifdef TARGET_I386

void ExportPE::initialize()
{
    m_baseDirs = s2e()->getConfig()->getStringList(getConfigKey() + ".baseDirs");

    m_exportInterval =
        s2e()->getConfig()->getInt(getConfigKey() + ".exportInterval", 0);

#if 0
    // print configuration for CPUArchState struct offsets
    errs() << "#define OFFSET_CR0            " << CPU_OFFSET(cr[0]) << '\n';
    errs() << "#define OFFSET_S2E_ICOUNT     " << CPU_OFFSET(s2e_icount) << '\n';
    errs() << "#define OFFSET_CURRENT_TB     " << CPU_OFFSET(current_tb) << '\n';
    errs() << "#define OFFSET_S2E_CURRENT_TB " << CPU_OFFSET(s2e_current_tb) << '\n';
    errs() << "#define OFFSET_SEG0_BASE      " << CPU_OFFSET(segs[0].base) << '\n';
    errs() << "#define OFFSET_SEG1_BASE      " << CPU_OFFSET(segs[1].base) << '\n';
    errs() << "#define OFFSET_SEG2_BASE      " << CPU_OFFSET(segs[2].base) << '\n';
    errs() << "#define OFFSET_SEG3_BASE      " << CPU_OFFSET(segs[3].base) << '\n';
    errs() << "#define OFFSET_SEG4_BASE      " << CPU_OFFSET(segs[4].base) << '\n';
    errs() << "#define OFFSET_SEG5_BASE      " << CPU_OFFSET(segs[5].base) << '\n';
    errs() << "#define OFFSET_CC_SRC         " << CPU_OFFSET(cc_src) << '\n';
    errs() << "#define OFFSET_CC_DST         " << CPU_OFFSET(cc_dst) << '\n';
    errs() << "#define OFFSET_CC_TMP         " << CPU_OFFSET(cc_tmp) << '\n';
    errs() << "#define OFFSET_CC_OP          " << CPU_OFFSET(cc_op) << '\n';
    errs() << "#define OFFSET_DF             " << CPU_OFFSET(df) << '\n';
    errs() << "#define OFFSET_MFLAGS         " << CPU_OFFSET(mflags) << '\n';
#endif

    m_selector = (ModuleSelector*)(s2e()->getPlugin("ModuleSelector"));
    PELibCallMonitor *libCallMonitor =
        (PELibCallMonitor*)(s2e()->getPlugin("PELibCallMonitor"));

    m_selector->onModuleLoad.connect(
            sigc::mem_fun(*this, &ExportPE::slotModuleLoad));
    m_selector->onModuleExecute.connect(
            sigc::mem_fun(*this, &ExportPE::slotModuleExecute));
    libCallMonitor->onLibCall.connect(
            sigc::mem_fun(*this, &ExportPE::slotLibCall));

    s2e()->getCorePlugin()->onStateFork.connect(
            sigc::mem_fun(*this, &ExportPE::slotStateFork));

    s2e()->getDebugStream() << "[ExportPE] Plugin initialized\n";
}

ExportPE::~ExportPE()
{
}

void ExportPE::slotModuleLoad(S2EExecutionState *state,
                              const ModuleDescriptor &module)
{
    initializeModule(module, m_baseDirs);

    bool success = loadSections(state);
    assert(success);
}

bool ExportPE::loadSections(S2EExecutionState *state)
{
    WindowsImage img(state, m_moduleDesc.LoadBase);
    ModuleSections sections = img.GetSections(state);

    LLVMContext &ctx = m_module->getContext();
    NamedMDNode *meta = m_module->getOrInsertNamedMetadata("sections");

    foreach2(it, sections.begin(), sections.end()) {
        const SectionDescriptor &section = *it;
        Value *operands[] = {
            MDString::get(ctx, section.name),
            ConstantInt::get(Type::getInt32Ty(ctx), section.loadBase),
            ConstantInt::get(Type::getInt32Ty(ctx), section.size),
            ConstantInt::get(Type::getInt32Ty(ctx), 0), // file offset
            NULL                                        // global
        };
        ArrayRef<Value*> opRef(operands);
        meta->addOperand(MDNode::get(ctx, opRef));
    }

    return !img.error();
}

void ExportPE::slotModuleExecute(S2EExecutionState *state, uint64_t pc)
{
    DECLARE_PLUGINSTATE(ExportPEState, state);
    exportBB(state, pc);
    addSuccessor(plgState->prevPc, pc);
    plgState->prevPc = pc;
}

void ExportPE::slotLibCall(S2EExecutionState *state,
        const ModuleDescriptor &lib, const std::string &routine,
        uint64_t pc, PELibCallMonitor::ReturnSignal &onReturn)
{
    LLVMContext &ctx = m_module->getContext();
    MDNode *md = MDNode::get(ctx, MDString::get(ctx, routine));
    getMetadataInst(pc)->setMetadata("extern_symbol", md);
}

void ExportPE::slotStateFork(S2EExecutionState *state,
        const std::vector<S2EExecutionState*> &newStates,
        const std::vector<klee::ref<klee::Expr> > &newCondition)
{
    stopRegeneratingBlocks();
}

#else

void ExportPE::initialize()
{
    s2e()->getMessagesStream() << "[ExportPE] This plugin is only suited for i386\n";
}

#endif

ExportPEState::ExportPEState() : prevPc(0) {}

ExportPEState::~ExportPEState() {}

} // namespace plugins
} // namespace s2e
