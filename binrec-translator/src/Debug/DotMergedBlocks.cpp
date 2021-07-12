#include "MetaUtils.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include <fstream>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

constexpr const char *FILENAME = "merged.dot";
constexpr int MAX_BLOCKS = 500;

namespace {
auto label(BasicBlock *bb) -> std::string {
    return '"' + intToHex(getBlockAddress(bb)) + " - " + intToHex(getLastPc(bb)) + '"';
}

/// S2E Save BB overlap information to merged.dot
class DotMergedBlocksPass : public PassInfoMixin<DotMergedBlocksPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        Function *wrapper = m.getFunction("Func_wrapper");
        assert(wrapper && "expected @wrapper() function");

        std::ofstream ofile(FILENAME);
        std::vector<BasicBlock *> succs, succs2;

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

        return PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addDotMergedBlocksPass(ModulePassManager &mpm) { mpm.addPass(DotMergedBlocksPass()); }
