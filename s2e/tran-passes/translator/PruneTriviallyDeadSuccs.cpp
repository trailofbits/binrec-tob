#include <set>
#include "PruneTriviallyDeadSuccs.h"
#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char PruneTriviallyDeadSuccs::ID = 0;
static RegisterPass<PruneTriviallyDeadSuccs> x("prune-trivially-dead-succs",
        "S2E Prune edges that are trivially unreachable from successor lists",
        false, false);

static void findTerminatingBlocks(BasicBlock *bb, std::set<BasicBlock*> &terms) {
    if (isa<ReturnInst>(bb->getTerminator()))
        terms.insert(bb);
    else
        foreach_succ(bb, succ, findTerminatingBlocks(succ, terms));
}

static inline bool isPcStore(Instruction &i) {
    ifcast(StoreInst, store, &i) {
        ifcast(GlobalVariable, glob, store->getPointerOperand())
            return glob->hasName() && glob->getName() == "PC";
    }

    return false;
}

static inline unsigned getConstPcStore(Instruction &i) {
    ifcast(ConstantInt, c, cast<StoreInst>(&i)->getValueOperand())
        return c->getZExtValue();

    return 0;
}

static inline BasicBlock *findSuccessor(std::vector<BasicBlock*> &bbs, unsigned pc) {
    for (BasicBlock *succ : bbs) {
        if (getBlockAddress(succ) == pc)
            return succ;
    }

    return NULL;
}

/*
 * Find basic blocks with successor lists that end with 'ret void' and check if
 * the last stored PC is constant. If so, remove other successors from the
 * successor list (and log the event).
 */
bool PruneTriviallyDeadSuccs::runOnModule(Module &m) {
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper);

    unsigned prunedSuccs = 0;

    for (BasicBlock &bb : *wrapper) {
        if (!isRecoveredBlock(&bb))
            continue;

        std::set<BasicBlock*> terminators;
        findTerminatingBlocks(&bb, terminators);

        for (BasicBlock *term : terminators) {
            unsigned lastStoredPc = 0;

            for (Instruction &i : *term) {
                if (isPcStore(i))
                    lastStoredPc = getConstPcStore(i);
            }

            if (lastStoredPc) {
                // We know the last stored PC, assert that is in the current
                // successor list and remove everything else from the list
                std::vector<BasicBlock*> succs;
                getBlockSuccs(&bb, succs);
                BasicBlock *onlySucc = findSuccessor(succs, lastStoredPc);

                if (!onlySucc) {
                    ERROR("block " << bb.getName() << " stores PC " <<
                        lastStoredPc << " but does not have BB_" <<
                        intToHex(lastStoredPc) << " in its successor list. Did you \
                        remember to disable multithreading in qemu (-smp 1)");
                    exit(1);
                }

                if (succs.size() == 1)
                    continue;

                prunedSuccs += succs.size() - 1;
                succs.clear();
                succs.push_back(onlySucc);
                setBlockSuccs(&bb, succs);

                DBG("set only successor of " << bb.getName() << " to " <<
                        onlySucc->getName());
            }
        }
    }

    INFO("pruned " << prunedSuccs << " trivially dead references from successor lists");

    return prunedSuccs > 0;
}
