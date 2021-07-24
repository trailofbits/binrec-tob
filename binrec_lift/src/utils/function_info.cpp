#include "function_info.hpp"
#include "pass_utils.hpp"
#include "binrec/tracing/trace_info.hpp"
#include <fstream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Module.h>

using namespace binrec;
using namespace llvm;
using namespace std;

FunctionInfo::FunctionInfo(const TraceInfo &ti)
{
    const FunctionLog &log = ti.functionLog;
    copy(log.entries.begin(), log.entries.end(), back_inserter(entry_pc));
    for (auto caller : log.entryToCaller) {
        entry_pc_to_caller_pcs[caller.first].insert(caller.second);
    }
    for (auto return_pc : log.entryToReturn) {
        entry_pc_to_return_pcs[return_pc.first].insert(return_pc.second);
    }
    for (auto follow_up : log.callerToFollowUp) {
        caller_pc_to_follow_up_pc.insert(follow_up);
    }
    for (auto &function : log.entryToTbs) {
        for (auto tb : function.second) {
            entry_pc_to_bb_pcs[function.first].insert(tb);
            bb_pc_to_entry_pcs[tb].insert(function.first);
        }
    }
}

auto FunctionInfo::get_tbs_by_function_entry(Module &m) const
    -> unordered_map<Function *, DenseSet<Function *>>
{
    unordered_map<Function *, DenseSet<Function *>> result;

    if (entry_pc_to_bb_pcs.empty()) {
        auto *entry = m.getFunction("Func_" + utohexstr(entry_pc[1]));
        assert(entry);
        DenseSet<Function *> tbs;
        for (Function &f : m) {
            if (f.getName().startswith("Func_") && f.getName() != "Func_wrapper") {
                tbs.insert(&f);
            }
        }
        result.emplace(entry, move(tbs));
    } else {
        for (auto &entry_to_tb_pcs : entry_pc_to_bb_pcs) {
            auto *entry = m.getFunction("Func_" + utohexstr(entry_to_tb_pcs.first));
            if (entry == nullptr) {
                continue;
            }
            DenseSet<Function *> tbs;
            for (uint32_t pc : entry_to_tb_pcs.second) {
                Function *f = m.getFunction("Func_" + utohexstr(pc));
                if (f != nullptr) {
                    tbs.insert(f);
                }
            }
            result.emplace(entry, move(tbs));
        }
    }

    return result;
}

auto FunctionInfo::get_ret_bbs_by_merged_function(const Module &m) const
    -> map<Function *, set<BasicBlock *>>
{
    map<Function *, set<BasicBlock *>> result;

    for (auto &entry_to_returns : entry_pc_to_return_pcs) {
        string merged_f_name = string("Func_") + utohexstr(entry_to_returns.first);
        Function *merged_f = m.getFunction(merged_f_name);
        if (merged_f == nullptr) {
            continue;
        }

        map<string, BasicBlock *> bb_by_name;
        for (BasicBlock &bb : *merged_f) {
            bb_by_name.emplace(bb.getName(), &bb);
        }

        set<BasicBlock *> return_blocks;
        transform(
            entry_to_returns.second.begin(),
            entry_to_returns.second.end(),
            inserter(return_blocks, return_blocks.begin()),
            [&bb_by_name](uint32_t block_pc) {
                return bb_by_name.find(string("BB_") + utohexstr(block_pc))->second;
            });

        result.emplace(merged_f, return_blocks);
    }

    return result;
}

auto FunctionInfo::get_entrypoint_functions(const Module &m) const -> vector<Function *>
{
    vector<Function *> entrypoints;
    transform(
        entry_pc.begin(),
        entry_pc.end(),
        back_inserter(entrypoints),
        [&m](uint32_t entrypoint_pc) {
            string entrypoint_name = string("Func_") + utohexstr(entrypoint_pc);
            Function *entrypoint = m.getFunction(entrypoint_name);
            return entrypoint;
        });
    return entrypoints;
}

auto FunctionInfo::get_ret_pcs_for_entry_pcs() const
    -> unordered_map<uint32_t, llvm::DenseSet<uint32_t>>
{
    return entry_pc_to_return_pcs;
}

auto FunctionInfo::get_caller_pc_to_follow_up_pc() const -> DenseMap<uint32_t, uint32_t>
{
    return caller_pc_to_follow_up_pc;
}
