/*
 * For each bb that has only one successor which has a "extern_symbol"
 * annotation, copy the successor and replace the label in the successor list.
 *
 * Also, replace the return value of helper_extern_call in the clone with a
 * dword load at R_ESP, so that optimization will be able to find a single
 * successor.
 */

#include <set>
#include "InlineStubs.h"
#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char InlineStubsPass::ID = 0;
static RegisterPass<InlineStubsPass> X("inline-stubs",
        "S2E Inline extern function stubs at each call to simplify CFG",
        false, false);

static inline bool isStub(BasicBlock &bb) {
    return getBlockMeta(&bb, "extern_symbol");
}

static inline bool isInlinedStub(BasicBlock &bb) {
    return isStub(bb) && bb.getName().count("from_");
}

bool InlineStubsPass::runOnModule(Module &m) {
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper);
    std::vector<BasicBlock*> succs;
    std::set<BasicBlock*> stubs, nonDead;
    unsigned callsites = 0;

    for (BasicBlock &bb : *wrapper) {
        if (!getBlockSuccs(&bb, succs))
            continue;

        // Don't inline recursively (happens when successor list of stub
        // contains stub itself, happens sometimes for some reason...)
        if (isInlinedStub(bb))
            continue;

        fori(i, 0, succs.size()) {
            BasicBlock *succ = succs[i];

            if (!isStub(*succ))
                continue;

            DBG("inlining stub " << succ->getName() << " after " <<
                    bb.getName());

            // clone BB
            ValueToValueMapTy vmap;
            std::string suffix(".from_" + bb.getName().str());
            BasicBlock *clone = CloneBasicBlock(succ, vmap, suffix, wrapper);

            // patch instruction references
            for (Instruction &inst : *clone) {
                RemapInstruction(&inst, vmap,
                        RF_NoModuleLevelChanges | RF_IgnoreMissingEntries);
            }

            stubs.insert(succ);

            // replace BB reference in successor list
            succs[i] = clone;
            setBlockSuccs(&bb, succs);

            callsites++;
        }
    }

    INFO("inlined " << stubs.size() << " stubs at " << callsites << " callsites");

    return callsites > 0;
}
