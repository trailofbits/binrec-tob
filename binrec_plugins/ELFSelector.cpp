#include "ELFSelector.h"
#include <s2e/S2E.h>
#include <s2e/S2EExecutor.h>

namespace s2e {
    namespace plugins {

        using namespace llvm;

        S2E_DEFINE_PLUGIN(
            ELFSelector,
            "Emits execution events only for blocks in an ELF binary",
            "ModuleSelector",
            "ModuleExecutionDetector");

        void ELFSelector::initialize()
        {
            m_moduleInitialized = false;

            ModuleExecutionDetector *modEx =
                (ModuleExecutionDetector *)(s2e()->getPlugin("ModuleExecutionDetector"));
            assert(modEx);

            modEx->onModuleLoad.connect(sigc::mem_fun(*this, &ELFSelector::slotModuleLoad));
            modEx->onModuleTranslateBlockStart.connect(
                sigc::mem_fun(*this, &ELFSelector::slotModuleTranslateBlockStart));
            s2e()->getCorePlugin()->onTranslateBlockStart.connect(
                sigc::mem_fun(*this, &ELFSelector::slotTranslateBlockStart));

            s2e()->getInfoStream() << "[ELFSelector] Plugin initialized\n";
        }

        void ELFSelector::slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module)
        {
            s2e()->getWarningsStream() << "[ELFSelector] slotModuleLoad: " << module.Name << "\n";
            // TODO (hbrodin): Is this the correct filename?
            // TODO (hbrodin): Updated s2e 'new_project' command automatically creates a filter for
            // the binary. Is this correct?
            if (module.Name == "init_env.so") {
                m_modInitEnv = module;
            } else {
                m_moduleInitialized = true;
                m_module = module;
                onModuleLoad.emit(state, module);
            }
        }

        void ELFSelector::slotTranslateBlockStart(
            ExecutionSignal *signal,
            S2EExecutionState *state,
            TranslationBlock *tb,
            uint64_t pc)
        {
            if (m_moduleInitialized) {
                signal->connect(sigc::mem_fun(*this, &ELFSelector::slotExecuteBlock));
            }
        }

        void ELFSelector::slotModuleTranslateBlockStart(
            ExecutionSignal *signal,
            S2EExecutionState *state,
            const ModuleDescriptor &module,
            TranslationBlock *tb,
            uint64_t pc)
        {
            // mark init_env.so blocks as external
            if (m_moduleInitialized && !m_modInitEnv.Contains(pc))
                signal->connect(sigc::mem_fun(*this, &ELFSelector::slotModuleExecuteBlock));
        }


        // NOTE (hbrodin): The order of invocation matters here.
        // slotExecutedBlock is executed before slotModuleExecuteBlock
        void ELFSelector::slotModuleExecuteBlock(S2EExecutionState *state, uint64_t pc)
        {
            // call onModuleReturn if a library call just returned
            // call onModuleExecute if a BB in the module is executed
            DECLARE_PLUGINSTATE(ELFSelectorState, state);

            if (plgState->inLibCall) {
                plgState->inLibCall = false;
                onModuleReturn.emit(state, pc);
            }

            onModuleExecute.emit(state, pc);
            plgState->prevPc = pc;
            plgState->prevPcInModule = true;
        }

        void ELFSelector::slotExecuteBlock(S2EExecutionState *state, uint64_t pc)
        {
            // call onModuleLeave on a library call
            DECLARE_PLUGINSTATE(ELFSelectorState, state);
            if (plgState->prevPcInModule) {
                // ignore library calls to init_env.so
                // TODO (hbrodin): Clean up this whole init_env thing, probably not needed
                // anymore since we can configure ModuleExecutionDetector to only signal
                // on load of the binary of interest.
                if (m_modInitEnv.Contains(pc)) {
                    s2e()->getDebugStream() << "ignoring init_env pc " << hexval(pc) << "\n";
                }
                // FIXME: this address keeps showing up...
                else if (pc == 0xc12c62ac)
                {
                    s2e()->getDebugStream() << "ignoring weird pc " << hexval(pc) << "\n";
                } else if (!m_module.Contains(pc)) {
                    plgState->inLibCall = true;
                    onModuleLeave.emit(state, pc);
                }
            }
            // Will be updated by slotModuleExecuteBlock if was actually in module,
            // if not this function will detect next time that we must be in a lib
            // call since we at some point was executing in the module.
            plgState->prevPcInModule = false;
        }

        uint64_t ELFSelector::getPrevLocalPc(S2EExecutionState *state)
        {
            DECLARE_PLUGINSTATE(ELFSelectorState, state);
            return plgState->prevPc;
        }

        /*
         * Execution-state-specific plugin state
         */

        ELFSelectorState::ELFSelectorState() : prevPcInModule(false), inLibCall(true), prevPc(0) {}

        ELFSelectorState::~ELFSelectorState() {}

    } // namespace plugins
} // namespace s2e
