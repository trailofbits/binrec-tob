#include <stdio.h>

#include "llvm/IR/Operator.h"

#include "RemoveQemuHelpers.h"
#include "PassUtils.h"
#include "DebugUtils.h"

using namespace llvm;

char RemoveQemuHelpers::ID = 0;
static RegisterPass<RemoveQemuHelpers> X("remove-qemu-helpers",
        "S2E Remove Qemu helper functions", false, false);

template <typename T>
static void findUsesIf(Value *v, std::list<Instruction*> &lst) {
    if (v) {
        for(auto user : v->users()) {
            if (Instruction *inst = dyn_cast<T>(user)){
                lst.push_back(inst);
            }else if(ConstantExpr *constExpr = dyn_cast<ConstantExpr>(user)){
                 if(constExpr->isCast())
                     findUsesIf<StoreInst>(user, lst);
            }
        }
    }
}

static void findCalls(Module &m, StringRef fnName, std::list<Instruction*> &lst) {
    findUsesIf<CallInst>(m.getFunction(fnName), lst);
}

static void findStores(Module &m, StringRef varName, std::list<Instruction*> &lst) {
    findUsesIf<StoreInst>(m.getGlobalVariable(varName), lst);
}

static void findConstPtrStores(Module &m, std::list<Instruction*> &lst) {
    uint64_t first_addr = 0;

    for (Function &f : m) {
        if (!isRecoveredBlock(&f))
            continue;

        for (BasicBlock &bb : f) {
            for (Instruction &inst : bb) {
                if (StoreInst *store = dyn_cast<StoreInst>(&inst)) {
                    if (Operator *op = dyn_cast<Operator>(store->getPointerOperand())) {
                        if (op->getOpcode() == Instruction::IntToPtr) {
                            ConstantInt *opnd = cast<ConstantInt>(op->getOperand(0));
                            assert(opnd->getType()->isIntegerTy(64) && "expected cast from 64-bit pointer");
                            assert(op->getType()->isPointerTy() && "expected cast to *i8");
                            //uint64_t addr = opnd->getZExtValue();

                            //if (first_addr)
                            //    assert(addr == first_addr && "expected inttoptr to always cast the same address");
                            //else
                            //    first_addr = addr;

                            lst.push_back(store);
                        }
                    }
                }
            }
        }
    }
}

/*
 * - remove calls to helper_{s2e_tcg_execution_handler,fninit,lock,unlock}
 * - replace calls to tcg_llvm_fork_and_concretize with the first
 *   operand(since we have no symbolic values), which can then be
 *   optimized away by constant propagation
 *
 * Remove stores to:
 * - @s2e_icount_after_tb (instruction counter)
 * - @s2e_current_tb      (translation block pointer)
 */
bool RemoveQemuHelpers::runOnModule(Module &m) {
    std::list<Instruction*> deleteList;

    findCalls(m, "tcg_llvm_fork_and_concretize", deleteList);
    for (Instruction *inst : deleteList)
        inst->replaceAllUsesWith(cast<CallInst>(inst)->getArgOperand(0));

    findCalls(m, "helper_s2e_tcg_execution_handler", deleteList);
    findCalls(m, "helper_lock", deleteList);
    findCalls(m, "helper_unlock", deleteList);

    findStores(m, "s2e_icount_before_tb", deleteList);
    findStores(m, "s2e_icount", deleteList);
    findStores(m, "s2e_current_tb", deleteList);

    findConstPtrStores(m, deleteList);

    for (Instruction *inst : deleteList)
        inst->eraseFromParent();

    return true;
}
