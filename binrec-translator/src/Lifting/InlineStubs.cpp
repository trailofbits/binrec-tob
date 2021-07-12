
#include "MetaUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <set>

using namespace binrec;
using namespace llvm;

namespace {
auto isStub(BasicBlock &bb) -> bool { return getBlockMeta(&bb, "extern_symbol"); }

auto isInlinedStub(BasicBlock &bb) -> bool { return isStub(bb) && bb.getName().count("from_"); }

/// S2E Inline extern function stubs at each call to simplify CFG
///
/// For each bb that has only one successor which has a "extern_symbol"
/// annotation, copy the successor and replace the label in the successor list.
///
/// Also, replace the return value of helper_extern_call in the clone with a
/// dword load at R_ESP, so that optimization will be able to find a single
/// successor.
class InlineStubsPass : public PassInfoMixin<InlineStubsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        bool Changed = false;
        for (Function &F : m) {
            if (!F.getName().startswith("Func_")) {
                continue;
            }
            std::vector<BasicBlock *> succs;
            std::set<BasicBlock *> stubs, nonDead;
            unsigned callsites = 0;

            for (BasicBlock &bb : F) {
                if (!getBlockSuccs(&bb, succs))
                    continue;

                // Don't inline recursively (happens when successor list of stub
                // contains stub itself, happens sometimes for some reason...)
                if (isInlinedStub(bb))
                    continue;

                for (unsigned i = 0, iUpperBound = succs.size(); i < iUpperBound; ++i) {
                    BasicBlock *succ = succs[i];

                    if (!isStub(*succ))
                        continue;

                    DBG("inlining stub " << succ->getName() << " after " << bb.getName());

                    // clone BB
                    ValueToValueMapTy vmap;
                    std::string suffix(".from_" + bb.getName().str());
                    BasicBlock *clone = CloneBasicBlock(succ, vmap, suffix, &F);

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

            Changed |= callsites > 0;
        }
        return Changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addInlineStubsPass(ModulePassManager &mpm) { mpm.addPass(InlineStubsPass()); }
