#include "UnalignStack.h"
#include "PassUtils.h"

using namespace llvm;

char UnalignStack::ID = 0;
static RegisterPass<UnalignStack> x("unalign-stack",
        "S2E Remove stack alignment of R_ESP",
        false, false);

static bool isStackAlign(Instruction &inst, GlobalVariable *R_ESP) {
    if (!inst.isBinaryOp() || inst.getOpcode() != Instruction::And)
        return false;

    ConstantInt *c = dyn_cast<ConstantInt>(inst.getOperand(1));

    if (!c || c->getSExtValue() != -16)
        return false;

    ifcast(LoadInst, load, inst.getOperand(0))
        return load->getPointerOperand() == R_ESP;

    // Check if left operand is is aligned add
    Instruction *left = dyn_cast<Instruction>(inst.getOperand(0));

    if (!left || !left->isBinaryOp() || left->getOpcode() != Instruction::Add)
        return false;

    c = dyn_cast<ConstantInt>(left->getOperand(1));

    if (!c || c->getSExtValue() % 16 != 0)
        return false;

    ifcast(LoadInst, load, left->getOperand(0))
        return load->getPointerOperand() == R_ESP;

    return false;
}

bool UnalignStack::runOnModule(Module &m) {
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper && "wrapper not found");

    // Remove stack alignment of esp
    GlobalVariable *R_ESP = m.getNamedGlobal("R_ESP");
    assert(R_ESP && "R_ESP not found");
    LoadInst *load;
    ConstantInt *c;
    StoreInst *store;

    for (Instruction &inst : wrapper->getEntryBlock()) {
        if (isStackAlign(inst, R_ESP)) {
            DBG("remove stack align instruction (assumes R_ESP is set to a 16-byte aligned address)");
            inst.replaceAllUsesWith(inst.getOperand(0));
            inst.eraseFromParent();
            return true;
        }
    }

    return false;
}

