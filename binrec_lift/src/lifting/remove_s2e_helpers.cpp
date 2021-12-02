#include "remove_s2e_helpers.hpp"
#include "ir/selectors.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/Operator.h>
#include <llvm/IR/PassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/ADT/SetVector.h>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
    template <typename T> void findUsesIf(Value *v, list<Instruction *> &lst)
    {
        if (v) {
            for (auto *user : v->users()) {
                if (Instruction *inst = dyn_cast<T>(user)) {
                    lst.push_back(inst);
                } else if (auto *constExpr = dyn_cast<ConstantExpr>(user)) {
                    if (constExpr->isCast())
                        findUsesIf<StoreInst>(user, lst);
                }
            }
        }
    }

    void findCalls(Module &m, StringRef fnName, list<Instruction *> &lst)
    {
        findUsesIf<CallInst>(m.getFunction(fnName), lst);
    }

    void findStores(Module &m, StringRef varName, list<Instruction *> &lst)
    {
        findUsesIf<StoreInst>(m.getGlobalVariable(varName), lst);
    }

    void findConstPtrStores(Module &m, list<Instruction *> &lst)
    {
        for (Function &f : LiftedFunctions{m}) {
            for (BasicBlock &bb : f) {
                for (Instruction &inst : bb) {
                    if (auto *store = dyn_cast<StoreInst>(&inst)) {
                        if (auto *op = dyn_cast<Operator>(store->getPointerOperand())) {
                            if (op->getOpcode() == Instruction::IntToPtr) {
                                auto *opnd = cast<ConstantInt>(op->getOperand(0));
                                assert(
                                    opnd->getType()->isIntegerTy(64) &&
                                    "expected cast from 64-bit pointer");
                                assert(op->getType()->isPointerTy() && "expected cast to *i8");
                                // uint64_t addr = opnd->getZExtValue();

                                // if (first_addr)
                                //    assert(addr == first_addr && "expected inttoptr to always cast
                                //    the same address");
                                // else
                                //    first_addr = addr;

                                lst.push_back(store);
                            }
                        }
                    }
                }
            }
        }
    }
} // namespace

// NOLINTNEXTLINE
auto RemoveS2EHelpersPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    list<Instruction *> deleteList;
    list<Instruction *> callscan;

    findCalls(m, "tcg_llvm_fork_and_concretize", deleteList);
    for (Instruction *inst : deleteList)
        inst->replaceAllUsesWith(cast<CallInst>(inst)->getArgOperand(0));


    findCalls(m, "tcg_llvm_before_memory_access", deleteList);
    findCalls(m, "tcg_llvm_after_memory_access", deleteList);
    findCalls(m, "helper_s2e_tcg_execution_handler", deleteList);

    findCalls(m, "helper_lock", deleteList);
    findCalls(m, "helper_unlock", deleteList);
    verifyModule(m, &errs());

    findStores(m, "s2e_icount_before_tb", deleteList);
    findStores(m, "s2e_icount", deleteList);
    findStores(m, "s2e_current_tb", deleteList);
    findStores(m, "se_current_tb", deleteList);

    findConstPtrStores(m, deleteList);

    for (Instruction *inst : deleteList) {
        inst->eraseFromParent();
    }

    for (Instruction *inst : callscan) {
        inst->eraseFromParent();
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
