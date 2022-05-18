#include "unalign_stack.hpp"
#include "error.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/PassManager.h>

#define PASS_NAME "unalign_stack"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;

static auto is_stack_align(Instruction &inst, GlobalVariable *r_esp) -> bool
{
    if (!inst.isBinaryOp() || inst.getOpcode() != Instruction::And)
        return false;

    auto *c = dyn_cast<ConstantInt>(inst.getOperand(1));

    if (!c || c->getSExtValue() != -16)
        return false;

    if (auto *load = dyn_cast<LoadInst>(inst.getOperand(0)))
        return load->getPointerOperand() == r_esp;

    // Check if left operand is is aligned add
    auto *left = dyn_cast<Instruction>(inst.getOperand(0));

    if (!left || !left->isBinaryOp() || left->getOpcode() != Instruction::Add)
        return false;

    c = dyn_cast<ConstantInt>(left->getOperand(1));

    if (!c || c->getSExtValue() % 16 != 0)
        return false;

    if (auto *load = dyn_cast<LoadInst>(left->getOperand(0)))
        return load->getPointerOperand() == r_esp;

    return false;
}

// NOLINTNEXTLINE
auto UnalignStackPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    bool changed = false;
    for (Function &f : m) {
        if (!f.getName().startswith("Func_")) {
            continue;
        }

        // Remove stack alignment of esp
        GlobalVariable *r_esp = m.getNamedGlobal("R_ESP");
        PASS_ASSERT(r_esp && "R_ESP not found");

        for (Instruction &inst : f.getEntryBlock()) {
            if (is_stack_align(inst, r_esp)) {
                DBG("remove stack align instruction (assumes R_ESP is set to a 16-byte aligned address)");
                inst.replaceAllUsesWith(inst.getOperand(0));
                inst.eraseFromParent();
                changed = true;
                break;
            }
        }
    }
    return changed ? PreservedAnalyses::allInSet<CFGAnalyses>() : PreservedAnalyses::all();
}
