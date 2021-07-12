#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/CFG.h>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

namespace {
/// S2E Remove stored of return address to the stack at libcalls
class PruneRetaddrPushPass : public PassInfoMixin<PruneRetaddrPushPass> {
public:
    /// From each libcall to helper_stub_trampoline, go to the predecessors of the
    /// containing block. The last virtual instruction in these blocks is marked by
    /// an !inststart instruction. Find the next store of a constant of (stored PC
    /// + 5) inside the instruction body and remove it.
    // NOLINTNEXTLINE
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        Function *helper = m.getFunction("helper_stub_trampoline");
        if (!helper)
            return PreservedAnalyses::all();

        for (Function &f : m) {
            if (!f.getName().startswith("Func_")) {
                continue;
            }

            unsigned pruneCount = 0, libCalls = 0;

            for (User *use : helper->users()) {
                auto *call = cast<CallInst>(use);
                BasicBlock *callee = call->getParent();

                if (callee->getParent() != &f)
                    continue;

                if (pred_begin(callee) == pred_end(callee)) {
                    DBG("not pruning return addresses for " << callee->getName() << ", no predecessors");
                    continue;
                }

                // Replace stack load at call with phi node
                CallInst *retaddrLoad = nullptr;

                for (BasicBlock::iterator i(call), e = callee->begin(); --i != e;) {
                    if (auto *ldlCall = dyn_cast<CallInst>(i)) {
                        Function *calledFunc = ldlCall->getCalledFunction();

                        if (calledFunc && calledFunc->getName() == "__ldl_mmu") {
                            retaddrLoad = ldlCall;
                            break;
                        }
                    }
                }

                assert(retaddrLoad);

                PHINode *phi = PHINode::Create(retaddrLoad->getType(), 10, "retaddr", &*(callee->begin()));
                retaddrLoad->replaceAllUsesWith(phi);
                retaddrLoad->eraseFromParent();

                for (BasicBlock *caller : predecessors(callee)) {
                    Instruction *retaddrStore = nullptr;

                    // Find last !inststart instruction
                    if (Instruction *lastInstStart = getLastInstStart(caller)) {
                        // From the last inststart, find the store of the return address and
                        // remove it
                        unsigned expectedRetaddr = getConstPcOperand(lastInstStart) + 5;

                        for (BasicBlock::iterator i(lastInstStart), e = caller->end(); ++i != e;) {
                            if (auto *push = dyn_cast<CallInst>(i)) {
                                Function *calledFunction = push->getCalledFunction();
                                if (calledFunction && calledFunction->getName() == "__stl_mmu") {
                                    if (auto *c = dyn_cast<ConstantInt>(push->getOperand(1))) {
                                        if (c->getZExtValue() == expectedRetaddr) {
                                            retaddrStore = push;
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                    }

                    Value *retaddrValue = nullptr;

                    if (retaddrStore) {
                        retaddrValue = retaddrStore->getOperand(1);
                        retaddrStore->eraseFromParent();
                    } else {
                        WARNING("could not find stored return address in " << caller->getName() << " for stub "
                                                                           << callee->getName() << ", assuming 0");
                        retaddrValue = ConstantInt::get(phi->getType(), 0);
                    }

                    // Replace stack store with global store
                    phi->addIncoming(retaddrValue, caller);

                    pruneCount++;
                }

                if (phi->getNumIncomingValues() == 0) {
                    errs() << "no phi values ";
                    exit(1);
                }

                libCalls++;
            }

            INFO("pruned " << pruneCount << " pushed return addresses at " << libCalls << " library calls");
        }

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addPruneRetaddrPushPass(ModulePassManager &mpm) { mpm.addPass(PruneRetaddrPushPass()); }
