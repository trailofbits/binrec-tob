#include "TagInstPc.h"
#include "MetaUtils.h"
#include "PassUtils.h"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto TagInstPcPass::run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
    GlobalVariable *PC = m.getNamedGlobal("PC");
    GlobalVariable *s2e_icount = m.getNamedGlobal("s2e_icount");
    assert(PC && s2e_icount && "globals missing");
    MDNode *md = MDNode::get(m.getContext(), NULL);

    for (User *use : s2e_icount->users()) {
        if (auto *icountStore = dyn_cast<StoreInst>(use)) {
            BasicBlock::iterator it(icountStore);
            StoreInst *pcStore = nullptr;
            while (!pcStore)
                pcStore = dyn_cast<StoreInst>(--it);
            assert(pcStore->getPointerOperand() == PC && "store before icount does not store to PC");
            pcStore->setMetadata("inststart", md);

            // update lastpc annotation on function
            unsigned pc = cast<ConstantInt>(pcStore->getValueOperand())->getZExtValue();
            Function *f = pcStore->getParent()->getParent();

            if (MDNode *mdlast = getBlockMeta(f, "lastpc")) {
                unsigned curlastpc =
                    cast<ConstantInt>(dyn_cast<ValueAsMetadata>(mdlast->getOperand(0))->getValue())->getZExtValue();
                if (curlastpc >= pc)
                    continue;
            }

            setBlockMeta(f, "lastpc", pc);
        }
    }

    return PreservedAnalyses::all();
}
