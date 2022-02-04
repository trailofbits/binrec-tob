#ifndef __PLUGIN_EXPORT_H__
#define __PLUGIN_EXPORT_H__

#include "binrec/tracing/trace_info.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>
#include <map>
#include <s2e/ConfigFile.h>
#include <s2e/Plugin.h>
#include <s2e/Plugins/OSMonitors/ModuleDescriptor.h>
#include <s2e/S2EExecutionState.h>
#include <set>
#include <string>

namespace s2e::plugins {
    class Export : public Plugin {
        using bb_count_t = std::map<uint64_t, unsigned int>;
        using bb_finalize_t = std::map<uint64_t, bool>;
        using bb_map_t = std::map<uint64_t, llvm::Function *>;

    protected:
        explicit Export(S2E *s2e);
        ~Export();

        void initialize();

        // LLVM module to export
        llvm::Module *m_module;
        // Descriptor of analysed module
        ModuleDescriptor m_moduleDesc;

        void
        initializeModule(const ModuleDescriptor &module, const ConfigFile::string_list &baseDirs);
        auto exportBB(S2EExecutionState *state, uint64_t pc) -> bool;
        void saveLLVMModule(bool intermediate);
        auto addSuccessor(uint64_t predPc, uint64_t pc) -> bool;
        auto getMetadataInst(uint64_t pc) -> llvm::Instruction *;
        auto getBB(uint64_t pc) -> llvm::Function *;

    private:
        // PC->LLVM cache
        bb_finalize_t m_bbFinalized;
        bb_count_t m_bbCounts;
        bb_map_t m_bbFuns;

        std::shared_ptr<binrec::TraceInfo> ti;

        unsigned m_exportCounter;

        void clearLLVMFunction(TranslationBlock *tb);
        auto areBBsEqual(llvm::Function *a, llvm::Function *b, bool *aIsValid, bool *bIsValid)
            -> bool;
        auto isFuncCallAndValid(llvm::Instruction *i) -> bool;
        void evaluateFunctions(
            llvm::Function *newFunc,
            llvm::Function *oldFunc,
            bool *aIsValid,
            bool *bIsValid);
        auto getExportFilename(bool intermediate) -> std::string;
        auto forceCodeGen(S2EExecutionState *state) -> llvm::Function *;
        auto regenCode(S2EExecutionState *state, llvm::Function *old) -> llvm::Function *;
        auto getFirstStoredPc(llvm::Function *f) -> uint64_t;
        bool m_regenerateBlocks;

    protected:
        unsigned m_exportInterval;

        virtual void stopRegeneratingBlocks();
    };

} // namespace s2e::plugins

#endif
