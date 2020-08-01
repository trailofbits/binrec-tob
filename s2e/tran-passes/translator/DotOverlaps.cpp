#include "DotOverlaps.h"
#include "PassUtils.h"
#include "MetaUtils.h"
#include "PcUtils.h"

using namespace llvm;

#define FILENAME "overlaps.dot"
#define ONLY_DIFF_END false
#define MAX_GROUPS 500

char DotOverlaps::ID = 0;
static RegisterPass<DotOverlaps> x("dot-overlaps",
        "S2E Save BB overlap information to " FILENAME, false, false);

static inline bool shouldDraw(BasicBlock *bb, BasicBlock *succ) {
    return succ != bb && blocksOverlap(bb, succ) &&
        (!ONLY_DIFF_END || !haveSameEndAddress(bb, succ));
}

static std::string label(BasicBlock *bb) {
    std::stringstream ss;
    ss << '"' << intToHex(getBlockAddress(bb)) << " - " <<
        intToHex(getLastPc(bb)) << '"';
    return ss.str();
}

bool DotOverlaps::runOnModule(Module &m) {
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper && "expected @wrapper() function");

    std::map<unsigned, std::vector<BasicBlock*>> bbMap;
    std::vector<BasicBlock*> succs;

    for (BasicBlock &bb : *wrapper) {
        if (isRecoveredBlock(&bb)) {
            unsigned lastpc = getLastPc(&bb);

            if (bbMap.find(lastpc) == bbMap.end())
                bbMap[lastpc] = std::vector<BasicBlock*>();

            bbMap[lastpc].push_back(&bb);
        }
    }

    std::ofstream ofile(FILENAME);
    ofile << "digraph overlaps {\n";
    ofile << "node [shape=box]\n";

    int groups = 0;

    for (auto it : bbMap) {
        const unsigned lastpc = it.first;
        const std::vector<BasicBlock*> &merge = it.second;

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
        const unsigned lastpc = it.first;
        const std::vector<BasicBlock*> &merge = it.second;

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

    return true;
}
