#include "add_mem_array.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;
using namespace std;

// NOLINTNEXTLINE
auto AddMemArrayPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    for (auto &f : m) {
        for (BasicBlock &bb : f) {
            list<CallInst *> loads, stores;

            for (Instruction &inst : bb) {
                if (auto *call = dyn_cast<CallInst>(&inst)) {
                    Function *calledFunction = call->getCalledFunction();

                    if (calledFunction && calledFunction->hasName()) {
                        StringRef name = calledFunction->getName();

                        if (name.endswith("_mmu")) {
                            if (name.startswith("helper_ld"))
                                loads.push_back(call);
                            else if (name.startswith("helper_st"))
                                stores.push_back(call);
                        }
                    }
                }
            }

            for (CallInst *call : loads) {
                Function *calledFunction = call->getCalledFunction();
                PointerType *ptrTy = calledFunction->getReturnType()->getPointerTo(0);

                if (!isa<ConstantInt>(call->getOperand(2)))
                    WARNING("weird mmu_idx:" << *call);

                IRBuilder<> builder(call);
                Value *ptr = builder.CreateIntToPtr(call->getOperand(1), ptrTy);
                auto *load = new LoadInst(call->getType(), ptr, "", call);
                call->replaceAllUsesWith(load);
                call->eraseFromParent();
            }

            for (CallInst *call : stores) {
                Value *target = call->getOperand(2);

                if (!isa<ConstantInt>(call->getOperand(3)))
                    WARNING("weird mmu_idx:" << *call);

                IRBuilder<> builder(call);
                Value *ptr =
                    builder.CreateIntToPtr(call->getOperand(1), target->getType()->getPointerTo());
                builder.CreateStore(target, ptr);
                call->eraseFromParent();
            }
        }
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
