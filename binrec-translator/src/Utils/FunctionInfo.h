#ifndef BINREC_FUNCTIONINFO_H
#define BINREC_FUNCTIONINFO_H

#include "Analysis/TraceInfoAnalysis.h"
#include <cstdint>
#include <llvm/IR/Module.h>
#include <map>
#include <set>
#include <vector>

namespace binrec {
struct FunctionInfo {
    std::vector<uint32_t> entryPc;
    std::map<uint32_t, std::set<uint32_t>> entryPcToReturnPcs;
    std::map<uint32_t, std::set<uint32_t>> entryPcToCallerPcs;
    std::map<uint32_t, uint32_t> callerPcToFollowUpPc;
    std::map<uint32_t, std::set<uint32_t>> entryPcToBBPcs;
    std::map<uint32_t, std::set<uint32_t>> BBPcToEntryPcs;

    explicit FunctionInfo(const TraceInfo &ti);

    [[nodiscard]] auto getTBsByFunctionEntry(const llvm::Module &M) const
        -> std::map<llvm::Function *, std::set<llvm::Function *>>;
    [[nodiscard]] auto getRetBBsByMergedFunction(const llvm::Module &M) const
        -> std::map<llvm::Function *, std::set<llvm::BasicBlock *>>;
    [[nodiscard]] auto getEntrypointFunctions(const llvm::Module &M) const -> std::vector<llvm::Function *>;
    [[nodiscard]] auto getRetPcsForEntryPcs() const -> std::map<uint32_t, std::set<uint32_t>>;
};
} // namespace binrec

#endif
