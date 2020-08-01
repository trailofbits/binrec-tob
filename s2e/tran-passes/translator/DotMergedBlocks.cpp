#include "DotMergedBlocks.h"
#include "PassUtils.h"
#include "MetaUtils.h"
#include "PcUtils.h"

using namespace llvm;

#define FILENAME "merged.dot"
#define MAX_BLOCKS 500

char DotMergedBlocks::ID = 0;
static RegisterPass<DotMergedBlocks> x("dot-merged-blocks",
        "S2E Save BB overlap information to " FILENAME, false, false);

static std::string label(BasicBlock *bb) {
    std::stringstream ss;
    ss << '"' << intToHex(getBlockAddress(bb)) << " - " <<
        intToHex(getLastPc(bb)) << '"';
    return ss.str();
}

bool DotMergedBlocks::runOnModule(Module &m) {
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper && "expected @wrapper() function");

    std::ofstream ofile(FILENAME);
    std::vector<BasicBlock*> succs, succs2;

    ofile << "digraph overlaps {\n";
    ofile << "node [shape=box]\n";

    int blocks = 0;

    for (BasicBlock &bb : *wrapper) {
        if (isRecoveredBlock(&bb) && getBlockMeta(&bb, "merged")) {
            ofile << label(&bb) << " [color=red]\n";
            getBlockSuccs(&bb, succs);

            for (BasicBlock *succ : succs)
                ofile << label(&bb) << " -> " << label(succ) << "\n";

            if (++blocks == MAX_BLOCKS)
                break;
        }
    }

    if (!blocks)
        ofile << "empty [shape=plaintext, label=\"no merged blocks\"]\n";

    ofile << "}\n";
    ofile.close();

    return true;
}
