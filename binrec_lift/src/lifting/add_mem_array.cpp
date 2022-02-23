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

                        // helper_*_mmu and cpu_(ld|st)* function are very similar:
                        // they load or store a value to/from a memory address. We can
                        // optimize both to a direct "store" instruction.
                        //
                        // Both sets of functions have the same beginning arguments:
                        //
                        //  - Load:
                        //    1. CPUX86State *env - does not exist at this point
                        //    2. target_ulong ptr - address
                        //
                        //  - Store:
                        //    1. CPUX86State *env - does not exist at this point
                        //    2. target_ulong ptr - address
                        //    3. T value - value
                        //
                        // The helper_*_mmu functions accept an additional argument,
                        // mmu_index, which is ignored in this pass.
                        if (name.endswith("_mmu")) {
                            // helper_*_mmu - load or store value
                            if (name.startswith("helper_ld"))
                                loads.push_back(call);
                            else if (name.startswith("helper_st"))
                                stores.push_back(call);
                        } else if (name.endswith("_data")) {
                            if (name.startswith("cpu_ld")) {
                                loads.push_back(call);
                            } else if (name.startswith("cpu_st")) {
                                stores.push_back(call);
                            }
                        }
                    }
                }
            }

            for (CallInst *call : loads) {
                Function *calledFunction = call->getCalledFunction();
                PointerType *ptrTy = calledFunction->getReturnType()->getPointerTo(0);

                if (call->arg_size() > 2 && !isa<ConstantInt>(call->getOperand(2)))
                    WARNING("weird mmu_idx:" << *call);

                IRBuilder<> builder(call);
                Value *ptr = builder.CreateIntToPtr(call->getOperand(1), ptrTy);
                auto *load = new LoadInst(call->getType(), ptr, "", call);
                call->replaceAllUsesWith(load);
                call->eraseFromParent();
            }

            for (CallInst *call : stores) {
                Value *target = call->getOperand(2);

                if (call->arg_size() > 3 && !isa<ConstantInt>(call->getOperand(3)))
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
