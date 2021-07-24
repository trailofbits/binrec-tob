#include "prune_retaddr_push.hpp"
#include "pass_utils.hpp"
#include "pc_utils.hpp"
#include <llvm/IR/CFG.h>

using namespace binrec;
using namespace llvm;

/// From each libcall to helper_stub_trampoline, go to the predecessors of the
/// containing block. The last virtual instruction in these blocks is marked by
/// an !inststart instruction. Find the next store of a constant of (stored PC
/// + 5) inside the instruction body and remove it.
// NOLINTNEXTLINE
auto PruneRetaddrPushPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    Function *helper = m.getFunction("helper_stub_trampoline");
    if (!helper)
        return PreservedAnalyses::all();

    for (Function &f : m) {
        if (!f.getName().startswith("Func_")) {
            continue;
        }

        unsigned prune_count = 0, lib_calls = 0;

        for (User *use : helper->users()) {
            auto *call = cast<CallInst>(use);
            BasicBlock *callee = call->getParent();

            if (callee->getParent() != &f)
                continue;

            if (pred_begin(callee) == pred_end(callee)) {
                DBG("not pruning return addresses for " << callee->getName()
                                                        << ", no predecessors");
                continue;
            }

            // Replace stack load at call with phi node
            CallInst *retaddr_load = nullptr;

            for (BasicBlock::iterator i(call), e = callee->begin(); --i != e;) {
                if (auto *ldl_call = dyn_cast<CallInst>(i)) {
                    Function *called_func = ldl_call->getCalledFunction();

                    if (called_func && called_func->getName() == "__ldl_mmu") {
                        retaddr_load = ldl_call;
                        break;
                    }
                }
            }

            assert(retaddr_load);

            PHINode *phi =
                PHINode::Create(retaddr_load->getType(), 10, "retaddr", &*(callee->begin()));
            retaddr_load->replaceAllUsesWith(phi);
            retaddr_load->eraseFromParent();

            for (BasicBlock *caller : predecessors(callee)) {
                Instruction *retaddr_store = nullptr;

                // Find last !inststart instruction
                if (Instruction *last_inst_start = getLastInstStart(caller)) {
                    // From the last inststart, find the store of the return address and
                    // remove it
                    unsigned expected_retaddr = getConstPcOperand(last_inst_start) + 5;

                    for (BasicBlock::iterator i(last_inst_start), e = caller->end(); ++i != e;) {
                        if (auto *push = dyn_cast<CallInst>(i)) {
                            Function *called_function = push->getCalledFunction();
                            if (called_function && called_function->getName() == "__stl_mmu") {
                                if (auto *c = dyn_cast<ConstantInt>(push->getOperand(1))) {
                                    if (c->getZExtValue() == expected_retaddr) {
                                        retaddr_store = push;
                                        break;
                                    }
                                }
                            }
                        }
                    }
                }

                Value *retaddr_value = nullptr;

                if (retaddr_store) {
                    retaddr_value = retaddr_store->getOperand(1);
                    retaddr_store->eraseFromParent();
                } else {
                    WARNING(
                        "could not find stored return address in "
                        << caller->getName() << " for stub " << callee->getName()
                        << ", assuming 0");
                    retaddr_value = ConstantInt::get(phi->getType(), 0);
                }

                // Replace stack store with global store
                phi->addIncoming(retaddr_value, caller);

                prune_count++;
            }

            if (phi->getNumIncomingValues() == 0) {
                errs() << "no phi values ";
                exit(1);
            }

            lib_calls++;
        }

        INFO(
            "pruned " << prune_count << " pushed return addresses at " << lib_calls
                      << " library calls");
    }

    return PreservedAnalyses::none();
}
