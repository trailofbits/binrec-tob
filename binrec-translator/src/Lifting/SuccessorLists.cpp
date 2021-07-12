#include "MetaUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include "binrec/tracing/TraceInfo.h"
#include <fstream>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

namespace {
/// S2E Create metadata annotations for successor lists
class SuccessorListsPass : public PassInfoMixin<SuccessorListsPass> {
    using BbCache = std::map<uint32_t, Function *>;

public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        TraceInfo ti;
        {
            std::ifstream f;
            s2eOpen(f, TraceInfo::defaultFilename);
            f >> ti;
        }

        BbCache bbCache;
        for (auto successor : ti.successors) {
            uint32_t pred = successor.pc;
            uint32_t succ = successor.successor;
            Function *predBB = getCachedFunc(bbCache, m, pred);

            if (!predBB) {
                // A block can be removed manually from the bitcode file
                continue;
            }

            Function *succBB = getCachedFunc(bbCache, m, succ);

            std::vector<Function *> succs;
            getBlockSuccs(predBB, succs);
            succs.push_back(succBB);
            setBlockSuccs(predBB, succs);
        }

        return PreservedAnalyses::all();
    }

private:
    static auto getCachedFunc(BbCache &bbCache, Module &m, uint32_t pc) -> Function * {
        auto cachedFunc = bbCache.find(pc);
        if (cachedFunc == bbCache.end()) {
            cachedFunc = bbCache.emplace(pc, m.getFunction("Func_" + intToHex(pc))).first;
        }
        return cachedFunc->second;
    }
};
} // namespace

void binrec::addSuccessorListsPass(ModulePassManager &mpm) { mpm.addPass(SuccessorListsPass()); }
