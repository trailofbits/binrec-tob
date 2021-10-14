#ifndef S2E_PLUGINS_BBEXPORT_H
#define S2E_PLUGINS_BBEXPORT_H

#include "Export.h"
#include "util.h"
#include <s2e/ConfigFile.h>
#include <s2e/Plugin.h>
#include <s2e/Plugins/CorePlugin.h>
#include <s2e/S2EExecutionState.h>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <stdio.h>

namespace s2e {
namespace plugins {

class BBExport : public Plugin {
    S2E_PLUGIN
public:
    BBExport(S2E *s2e) : Plugin(s2e) {}
    void initialize();

    ~BBExport();

    void slotModuleLoad(S2EExecutionState *state, const ModuleDescriptor &module);
    void slotTranslateBlockStart(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb, uint64_t pc);
    void slotTranslateBlockEnd(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb, uint64_t pc,
                               bool staticTargetValid, uint64_t staticTarget);
    void slotModuleExecuteFirst(S2EExecutionState *state, uint64_t pc);
    void slotModuleExecuteBlock(S2EExecutionState *state, uint64_t pc);
    void saveLLVMModule(Function &F);
    uint64_t extractSuccEdge(Function &F);
    void slotModuleExecute(S2EExecutionState *state, uint64_t pc);
    void addEdge(uint64_t predPc, uint64_t succ);
    void saveLLVMModule();
    void readPrevSuccs(std::string &path);
    void readSymbols(std::string &path);
    void suspend(ExecutionSignal *signal, S2EExecutionState *state, TranslationBlock *tb, uint64_t pc);

private:
    sigc::connection m_tbStartConnection;
    sigc::connection m_tbEndConnection;

    uint64_t m_pred, m_address, m_currentBlock;
    bool m_moduleLoaded;
    bool m_executionDiverted;
    ModuleDescriptor m_module;
    // ExecutionSignal *m_startSignal;
    std::set<uint64_t> m_succs;
    llvm::Module *m_llvmModule;
    bool m_earlyTerminate;
    std::set<uint32_t> prevSuccsKeys;
    std::set<uint32_t> symbolsSet;
    bool m_suspend;
};

} // namespace plugins
} // namespace s2e

#endif
