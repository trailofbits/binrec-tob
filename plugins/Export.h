#ifndef __PLUGIN_EXPORT_H__
#define __PLUGIN_EXPORT_H__

#include <string>
#include <map>

#include <s2e/Plugin.h>
#include <s2e/ConfigFile.h>
#include <s2e/S2EExecutionState.h>
#include <s2e/Plugins/ModuleDescriptor.h>

#include <llvm/Module.h>
#include <llvm/Function.h>

namespace s2e {
namespace plugins {

using namespace llvm;

/**
 *  Base class for binary file code selectors
 */
class Export : public Plugin
{
    typedef std::map<uint64_t, unsigned> bb_count_t;
    typedef std::map<uint64_t, bool> bb_finalize_t;
    typedef std::map<uint64_t, Function*> bb_map_t;
    typedef std::set<uint64_t> succs_map_t;

#ifdef TARGET_I386

protected:
    Export(S2E* s2e);
    ~Export();

    // LLVM module to export
    Module *m_module;
    // Descriptor of analysed module
    ModuleDescriptor m_moduleDesc;

    void initializeModule(const ModuleDescriptor &module,
            const ConfigFile::string_list &baseDirs);
    bool exportBB(S2EExecutionState *state, uint64_t pc);
    void saveLLVMModule(bool intermediate);
    bool addSuccessor(uint64_t predPc, uint64_t pc);
    Instruction *getMetadataInst(uint64_t pc);
    Function *getBB(uint64_t pc);

private:
    // PC->LLVM cache
    bb_finalize_t m_bbFinalized;
    bb_count_t m_bbCounts;
    bb_map_t m_bbFuns;

    succs_map_t m_succs;

    unsigned m_exportCounter;

    void clearLLVMFunction(TranslationBlock *tb);
    bool areBBsEqual(Function *a, Function *b, bool *aIsValid, bool *bIsValid);
    bool isFuncCallAndValid(Instruction *I);
    void evaluateFunctions(Function *a, Function *b/*old Func*/, bool *aIsValid, bool *bIsValid); 
    std::string getExportFilename(bool intermediate);
    Function *forceCodeGen(S2EExecutionState *state);
    Function *regenCode(S2EExecutionState *state, Function *old);

    bool m_regenerateBlocks;

protected:
    unsigned m_exportInterval;

    virtual void stopRegeneratingBlocks();

#else // TARGET_I386

protected:
    Export(S2E* s2e) : Plugin(s2e) {}

#endif
};

} // namespace plugins
} // namespace s2e

#endif // __PLUGIN_EXPORT_H__
