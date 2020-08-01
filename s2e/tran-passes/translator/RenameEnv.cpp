#include "RenameEnv.h"
#include "PassUtils.h"

using namespace llvm;

char RenameEnv::ID = 0;
static RegisterPass<RenameEnv> X("rename-env",
        "S2E Rename globals from env struct to match what the next passes expect",
        false, false);

static const char* const regNames[] = {
    "R_EAX", "R_ECX", "R_EDX", "R_EBX", "R_ESP", "R_EBP", "R_ESI", "R_EDI"
};

template<typename T>
static void replaceGEP(Module &m, T *gep, Type *regTy) {
    assert(gep->getNumIndices() == 2);
    assert(gep->hasAllConstantIndices());
    unsigned index = cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
    assert(index < 8);
    gep->replaceAllUsesWith(cast<GlobalVariable>(
        m.getOrInsertGlobal(regNames[index], regTy)));
}

/*
 */
bool RenameEnv::runOnModule(Module &m) {
    m.getNamedGlobal("eip")->setName("PC");

    GlobalVariable *regs = m.getNamedGlobal("regs");
    Type *regTy = cast<ArrayType>(regs->getType()->getElementType())->getElementType();
    foreach_use_if(regs, GetElementPtrInst, gep, replaceGEP(m, gep, regTy));
    foreach_use_as(regs, GEPOperator, gep, replaceGEP(m, gep, regTy));

    return true;
}
