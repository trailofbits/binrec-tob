#include "MetaUtils.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include <fstream>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

constexpr const char *FILENAME = "overlaps.dot";
constexpr int MAX_GROUPS = 500;

namespace {
auto label(BasicBlock *bb) -> std::string {
    return '"' + intToHex(getBlockAddress(bb)) + " - " + intToHex(getLastPc(bb)) + '"';
}

/// S2E Save BB overlap information to overlaps.dot
class DotOverlapsPass : public PassInfoMixin<DotOverlapsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        Function *wrapper = m.getFunction("Func_wrapper");
        assert(wrapper && "expected @wrapper() function");

        std::map<unsigned, std::vector<BasicBlock *>> bbMap;
        std::vector<BasicBlock *> succs;

        for (BasicBlock &bb : *wrapper) {
            if (isRecoveredBlock(&bb)) {
                unsigned lastpc = getLastPc(&bb);

                if (bbMap.find(lastpc) == bbMap.end())
                    bbMap[lastpc] = std::vector<BasicBlock *>();

                bbMap[lastpc].push_back(&bb);
            }
        }

        std::ofstream ofile(FILENAME);
        ofile << "digraph overlaps {\n";
        ofile << "node [shape=box]\n";

        int groups = 0;

        for (auto it : bbMap) {
            const unsigned lastpc = it.first;
            const std::vector<BasicBlock *> &merge = it.second;

            if (merge.size() >= 2) {
                ofile << "subgraph cluster_" << intToHex(lastpc) << " {\n";
                ofile << "color=grey\n";

                for (BasicBlock *bb : merge)
                    ofile << label(bb) << " [color=red]\n";

                ofile << "}\n";

                if (++groups == MAX_GROUPS)
                    break;
            }
        }

        groups = 0;

        for (auto it : bbMap) {
            const std::vector<BasicBlock *> &merge = it.second;

            if (merge.size() >= 2) {
                for (BasicBlock *bb : merge) {
                    getBlockSuccs(bb, succs);

                    for (BasicBlock *succ : succs)
                        ofile << label(bb) << " -> " << label(succ) << "\n";
                }

                if (++groups == MAX_GROUPS)
                    break;
            }
        }

        if (!groups)
            ofile << "empty [shape=plaintext, label=\"no overlaps\"]\n";

        ofile << "}\n";
        ofile.close();

        return PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addDotOverlapsPass(ModulePassManager &mpm) { mpm.addPass(DotOverlapsPass()); }
