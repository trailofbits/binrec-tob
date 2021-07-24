
#include "inline_stubs.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <llvm/Transforms/Utils/Cloning.h>
#include <set>

using namespace binrec;
using namespace llvm;
using namespace std;

static auto is_stub(BasicBlock &bb) -> bool
{
    return getBlockMeta(&bb, "extern_symbol");
}

static auto is_inlined_stub(BasicBlock &bb) -> bool
{
    return is_stub(bb) && bb.getName().count("from_");
}

// NOLINTNEXTLINE
auto InlineStubsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    bool changed = false;
    for (Function &f : m) {
        if (!f.getName().startswith("Func_")) {
            continue;
        }
        vector<BasicBlock *> succs;
        set<BasicBlock *> stubs, non_dead;
        unsigned callsites = 0;

        for (BasicBlock &bb : f) {
            if (!getBlockSuccs(&bb, succs))
                continue;

            // Don't inline recursively (happens when successor list of stub
            // contains stub itself, happens sometimes for some reason...)
            if (is_inlined_stub(bb))
                continue;

            for (unsigned i = 0, i_upper_bound = succs.size(); i < i_upper_bound; ++i) {
                BasicBlock *succ = succs[i];

                if (!is_stub(*succ))
                    continue;

                DBG("inlining stub " << succ->getName() << " after " << bb.getName());

                // clone BB
                ValueToValueMapTy vmap;
                string suffix(".from_" + bb.getName().str());
                BasicBlock *clone = CloneBasicBlock(succ, vmap, suffix, &f);

                // patch instruction references
                for (Instruction &inst : *clone) {
                    RemapInstruction(&inst, vmap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);
                }

                stubs.insert(succ);

                // replace BB reference in successor list
                succs[i] = clone;
                setBlockSuccs(&bb, succs);

                callsites++;
            }
        }

        INFO("inlined " << stubs.size() << " stubs at " << callsites << " callsites");

        changed |= callsites > 0;
    }
    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
