#include "PruneRetaddrPush.h"
#include "PassUtils.h"
#include "PcUtils.h"

using namespace llvm;

char PruneRetaddrPush::ID = 0;
static RegisterPass<PruneRetaddrPush> x("prune-retaddr-push",
        "S2E Remove stored of return address to the stack at libcalls",
        false, false);

/*
 * From each libcall to helper_stub_trampoline, go to the predecessors of the
 * containing block. The last virtual instruction in these blocks is marked by
 * an !inststart instruction. Find the next store of a constant of (stored PC +
 * 5) inside the instruction body and remove it.
 */
bool PruneRetaddrPush::runOnModule(Module &m) {
    Function *helper = m.getFunction("helper_stub_trampoline");
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper);

    if (!helper)
        return false;

    unsigned pruneCount = 0, libCalls = 0;

    foreach_use_as(helper, CallInst, call, {
        BasicBlock *callee = call->getParent();

        if (callee->getParent() != wrapper)
            continue;

        if (pred_begin(callee) == pred_end(callee)) {
            DBG("not pruning return addresses for " << callee->getName() <<
                ", no predecessors");
            continue;
        }

        // Replace stack load at call with phi node
        CallInst *retaddrLoad = NULL;

        for (BasicBlock::iterator i(call), e = callee->begin(); --i != e;) {
            ifcast(CallInst, ldlCall, i) {
                Function *f = ldlCall->getCalledFunction();

                if (f && f->getName() == "__ldl_mmu") {
                    retaddrLoad = ldlCall;
                    break;
                }
            }
        }

        assert(retaddrLoad);

        PHINode *phi = PHINode::Create(retaddrLoad->getType(), 10, "retaddr", &*(callee->begin()));
        retaddrLoad->replaceAllUsesWith(phi);
        retaddrLoad->eraseFromParent();

        foreach_pred(callee, caller, {
            Instruction *retaddrStore = NULL;

            // Find last !inststart instruction
            if (Instruction *lastInstStart = getLastInstStart(caller)) {
                // From the last inststart, find the store of the return address and
                // remove it
                unsigned expectedRetaddr = getConstPcOperand(lastInstStart) + 5;

                for (BasicBlock::iterator i(lastInstStart), e = caller->end(); ++i != e;) {
                    ifcast(CallInst, push, i) {
                        Function *f = push->getCalledFunction();
                        if (f && f->getName() == "__stl_mmu") {
                            ifcast(ConstantInt, c, push->getOperand(1)) {
                                if (c->getZExtValue() == expectedRetaddr) {
                                    retaddrStore = push;
                                    break;
                                }
                            }
                        }
                    }
                }
            }

            Value *retaddrValue;

            if (retaddrStore) {
                retaddrValue = retaddrStore->getOperand(1);
                retaddrStore->eraseFromParent();
            } else {
                WARNING("could not find stored return address in " <<
                        caller->getName() << " for stub " << callee->getName() <<
                        ", assuming 0");
                retaddrValue = ConstantInt::get(phi->getType(), 0);
            }

            // Replace stack store with global store
            phi->addIncoming(retaddrValue, caller);

            pruneCount++;
        });

        if (phi->getNumIncomingValues() == 0) {
            errs() << "no phi values ";
            callee->dump();
            exit(1);
        }

        libCalls++;
    });

    INFO("pruned " << pruneCount << " pushed return addresses at " <<
            libCalls << " library calls");

    return true;
}
