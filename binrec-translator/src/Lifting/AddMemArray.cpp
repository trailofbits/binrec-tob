#include "AddMemArray.h"
#include "PassUtils.h"

using namespace binrec;
using namespace llvm;
using namespace std;

// NOLINTNEXTLINE
auto AddMemArrayPass::run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
    for (auto &f : m) {
        for (BasicBlock &bb : f) {
            list<CallInst *> loads, stores;

            for (Instruction &inst : bb) {
                if (auto *call = dyn_cast<CallInst>(&inst)) {
                    Function *calledFunction = call->getCalledFunction();

                    if (calledFunction && calledFunction->hasName()) {
                        StringRef name = calledFunction->getName();

                        if (name.endswith("_mmu")) {
                            if (name.startswith("__ld"))
                                loads.push_back(call);
                            else if (name.startswith("__st"))
                                stores.push_back(call);
                        }
                    }
                }
            }

            for (CallInst *call : loads) {
                Function *calledFunction = call->getCalledFunction();
                PointerType *ptrTy = calledFunction->getReturnType()->getPointerTo(0);

                if (!isa<ConstantInt>(call->getOperand(1)))
                    WARNING("weird mmu_idx:" << *call);

                IRBuilder<> builder(call);
                Value *ptr = builder.CreateIntToPtr(call->getOperand(0), ptrTy);
                auto *load = new LoadInst(call->getType(), ptr, "", call);
                call->replaceAllUsesWith(load);
                call->eraseFromParent();
            }

            for (CallInst *call : stores) {
                Value *target = call->getOperand(1);

                if (!isa<ConstantInt>(call->getOperand(2)))
                    WARNING("weird mmu_idx:" << *call);

                IRBuilder<> builder(call);
                Value *ptr = builder.CreateIntToPtr(call->getOperand(0), target->getType()->getPointerTo());
                builder.CreateStore(target, ptr);
                call->eraseFromParent();
            }
        }
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
