#include "successor_lists.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "binrec/tracing/trace_info.hpp"
#include <fstream>

using namespace binrec;
using namespace llvm;

static auto get_cached_func(std::map<uint32_t, Function *> &bb_cache, Module &m, uint32_t pc)
    -> Function *
{
    auto cached_func = bb_cache.find(pc);
    if (cached_func == bb_cache.end()) {
        cached_func = bb_cache.emplace(pc, m.getFunction("Func_" + utohexstr(pc))).first;
    }
    return cached_func->second;
}

// NOLINTNEXTLINE
auto SuccessorListsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    TraceInfo ti;
    {
        std::ifstream f;
        s2eOpen(f, TraceInfo::defaultFilename);
        f >> ti;
    }

    std::map<uint32_t, Function *> bb_cache;
    for (auto successor : ti.successors) {
        uint32_t pred = successor.pc;
        uint32_t succ = successor.successor;
        Function *pred_bb = get_cached_func(bb_cache, m, pred);

        if (!pred_bb) {
            // A block can be removed manually from the bitcode file
            continue;
        }

        Function *succ_bb = get_cached_func(bb_cache, m, succ);

        std::vector<Function *> succs;
        getBlockSuccs(pred_bb, succs);
        succs.push_back(succ_bb);
        setBlockSuccs(pred_bb, succs);
    }
    return PreservedAnalyses::all();
}
