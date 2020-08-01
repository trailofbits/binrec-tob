#include "AddMemArray.h"
#include "PassUtils.h"

using namespace llvm;

char AddMemArrayPass::ID = 0;
static RegisterPass<AddMemArrayPass> X("add-mem-array",
        "S2E replace memory access helpers with pointer dereferences", false, false);

/*
 * Add a global byte array 'memory'
 */
bool AddMemArrayPass::doInitialization(Module &m) {
    Type *byteTy = Type::getInt8Ty(m.getContext());
    memory = new GlobalVariable(m, byteTy, false,
            GlobalValue::ExternalLinkage, NULL, MEMNAME);
    // TODO: see if inbounds getelementptr works differently
    //memory  = new GlobalVariable(m, ArrayType::get(byteTy, MEMSIZE), false,
    //        GlobalValue::ExternalLinkage, NULL, MEMNAME);
    //memory->setExternallyInitialized(true);
    return true;
}

/*
 * Replace calls to __ldX_mmu/__stX_mmu helpers with loads/stores to an element
 * pointer in @memory:
 * - %x = __ldX_mmu(addr, 1)  -->  %memptr = getelementptr @memory, addr
 *                                 %ptr = bitcast i8* %memptr to iX*
 *                                 %x = load %ptr
 * - __stX_mmu(addr, %x, 1)   -->  %memptr = getelementptr @memory, addr
 *                                 %ptr = bitcast i8* %memptr to iX*
 *                                 store %x, %ptr
 */
bool AddMemArrayPass::runOnBasicBlock(BasicBlock &bb) {
    std::list<CallInst*> loads, stores;

    foreach(inst, bb) {
        if (CallInst *call = dyn_cast<CallInst>(inst)) {
            Function *f = call->getCalledFunction();

            if (f && f->hasName()) {
                StringRef name = f->getName();

                if (name.endswith("_mmu")) {
                    if (name.startswith("__ld"))
                        loads.push_back(call);
                    else if (name.startswith("__st"))
                        stores.push_back(call);
                }
            }
        }
    }

    foreach(it, loads) {
        CallInst *call = *it;
        Function *f = call->getCalledFunction();
        PointerType *ptrTy = f->getReturnType()->getPointerTo(0);

        if (!isa<ConstantInt>(call->getOperand(1)))
            WARNING("weird mmu_idx:" << *call);

        IRBuilder<> builder(call);
        Value *arrayPtr = builder.CreateInBoundsGEP(memory, call->getOperand(0));
        Value *ptr = builder.CreatePointerCast(arrayPtr, ptrTy);
        ReplaceInstWithInst(call, new LoadInst(ptr, ""));
    }

    foreach(it, stores) {
        CallInst *call = *it;
        Value *target = call->getOperand(1);

        if (!isa<ConstantInt>(call->getOperand(2)))
            WARNING("weird mmu_idx:" << *call);

        IRBuilder<> builder(call);
        Value *arrayPtr = builder.CreateInBoundsGEP(memory, call->getOperand(0));
        Value *ptr = builder.CreatePointerCast(arrayPtr, target->getType()->getPointerTo(0));
        builder.CreateStore(target, ptr);
        call->eraseFromParent();
    }

    return true;
}
