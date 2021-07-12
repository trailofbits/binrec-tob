#include "MainUtils.h"
#include "PassUtils.h"

static auto getOrInsertMalloc(Module *m) -> FunctionCallee {
    LLVMContext &ctx = m->getContext();
    return m->getOrInsertFunction("malloc", Type::getInt8Ty(ctx)->getPointerTo(0), Type::getInt32Ty(ctx));
}

static auto getOrInsertFree(Module *m) -> FunctionCallee {
    LLVMContext &ctx = m->getContext();
    return m->getOrInsertFunction("free", Type::getVoidTy(ctx), Type::getInt8Ty(ctx)->getPointerTo(0));
}

void callWrapper(BasicBlock *bb) {
    Module *m = bb->getParent()->getParent();
    CallInst::Create(m->getFunction("Func_wrapper"), "", bb);
}

auto allocateStack(BasicBlock *bb) -> Value * {
    Module *m = bb->getParent()->getParent();
    IntegerType *addrTy = Type::getInt32Ty(bb->getContext());
    ConstantInt *stackSize = ConstantInt::get(addrTy, STACK_SIZE);
    Value *stack = CallInst::Create(getOrInsertMalloc(m), stackSize, "stack", bb);
    Value *stackAddr = new PtrToIntInst(stack, addrTy, "", bb);
    Value *stackEnd = BinaryOperator::Create(Instruction::Add, stackAddr, stackSize, "", bb);
    new StoreInst(stackEnd, m->getGlobalVariable("R_ESP"), bb);
    return stack;
}

void freeStack(Value *stack, BasicBlock *bb) {
    Module *m = bb->getParent()->getParent();
    CallInst::Create(getOrInsertFree(m), stack, "", bb);
}
