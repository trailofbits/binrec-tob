#include "FunctionInfo.h"
#include "PassUtils.h"
#include "binrec/tracing/TraceInfo.h"
#include <fstream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

using namespace binrec;
using namespace llvm;

FunctionInfo::FunctionInfo(const TraceInfo &ti) {
    const FunctionLog &log = ti.functionLog;
    std::copy(log.entries.begin(), log.entries.end(), std::back_inserter(entryPc));
    for (auto caller : log.entryToCaller) {
        entryPcToCallerPcs[caller.first].insert(caller.second);
    }
    for (auto returnPc : log.entryToReturn) {
        entryPcToReturnPcs[returnPc.first].insert(returnPc.second);
    }
    for (auto followUp : log.callerToFollowUp) {
        callerPcToFollowUpPc.insert(followUp);
    }
    for (auto &function : log.entryToTbs) {
        for (auto tb : function.second) {
            entryPcToBBPcs[function.first].insert(tb);
            BBPcToEntryPcs[tb].insert(function.first);
        }
    }
}

auto FunctionInfo::getTBsByFunctionEntry(const Module &M) const -> std::map<Function *, std::set<Function *>> {
    std::map<Function *, std::set<Function *>> Result;

    for (auto &EntryToTBPcs : entryPcToBBPcs) {
        std::string TBName = std::string("Func_") + intToHex(EntryToTBPcs.first);
        Function *Entry = M.getFunction(TBName);
        if (Entry == nullptr) {
            continue;
        }
        std::set<Function *> TBs;
        std::transform(EntryToTBPcs.second.begin(), EntryToTBPcs.second.end(), std::inserter(TBs, TBs.begin()),
                       [&M](uint32_t PC) { return M.getFunction(std::string("Func_") + intToHex(PC)); });
        TBs.erase(nullptr);
        Result.emplace(Entry, move(TBs));
    }

    return Result;
}

auto FunctionInfo::getRetBBsByMergedFunction(const Module &M) const -> std::map<Function *, std::set<BasicBlock *>> {
    std::map<Function *, std::set<BasicBlock *>> Result;

    for (auto &EntryToReturns : entryPcToReturnPcs) {
        std::string MergedFName = std::string("Func_") + intToHex(EntryToReturns.first);
        Function *MergedF = M.getFunction(MergedFName);
        if (MergedF == nullptr) {
            continue;
        }

        std::map<std::string, BasicBlock *> BBByName;
        for (BasicBlock &BB : *MergedF) {
            BBByName.emplace(BB.getName(), &BB);
        }

        std::set<BasicBlock *> ReturnBlocks;
        std::transform(
            EntryToReturns.second.begin(), EntryToReturns.second.end(), inserter(ReturnBlocks, ReturnBlocks.begin()),
            [&BBByName](uint32_t BlockPC) { return BBByName.find(std::string("BB_") + intToHex(BlockPC))->second; });

        Result.emplace(MergedF, ReturnBlocks);
    }

    return Result;
}

auto FunctionInfo::getEntrypointFunctions(const Module &M) const -> std::vector<Function *> {
    std::vector<Function *> entrypoints;
    std::transform(entryPc.begin(), entryPc.end(), std::back_inserter(entrypoints), [&M](uint32_t entrypointPc) {
        std::string entrypointName = std::string("Func_") + intToHex(entrypointPc);
        Function *entrypoint = M.getFunction(entrypointName);
        assert(entrypoint && "Cannot locate entrypoint function.");
        return entrypoint;
    });
    return entrypoints;
}

auto FunctionInfo::getRetPcsForEntryPcs() const -> std::map<uint32_t, std::set<uint32_t>> {
        return entryPcToReturnPcs;
}
