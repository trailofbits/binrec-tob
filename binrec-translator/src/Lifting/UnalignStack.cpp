#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
auto isStackAlign(Instruction &inst, GlobalVariable *R_ESP) -> bool {
    if (!inst.isBinaryOp() || inst.getOpcode() != Instruction::And)
        return false;

    auto *c = dyn_cast<ConstantInt>(inst.getOperand(1));

    if (!c || c->getSExtValue() != -16)
        return false;

    if (auto *load = dyn_cast<LoadInst>(inst.getOperand(0)))
        return load->getPointerOperand() == R_ESP;

    // Check if left operand is is aligned add
    auto *left = dyn_cast<Instruction>(inst.getOperand(0));

    if (!left || !left->isBinaryOp() || left->getOpcode() != Instruction::Add)
        return false;

    c = dyn_cast<ConstantInt>(left->getOperand(1));

    if (!c || c->getSExtValue() % 16 != 0)
        return false;

    if (auto *load = dyn_cast<LoadInst>(left->getOperand(0)))
        return load->getPointerOperand() == R_ESP;

    return false;
}

/// S2E Remove stack alignment of R_ESP
class UnalignStackPass : public PassInfoMixin<UnalignStackPass> {
public:
    auto run(Module &M, ModuleAnalysisManager &) -> PreservedAnalyses {
        bool Changed = false;
        for (Function &F : M) {
            if (!F.getName().startswith("Func_")) {
                continue;
            }

            // Remove stack alignment of esp
            GlobalVariable *R_ESP = M.getNamedGlobal("R_ESP");
            assert(R_ESP && "R_ESP not found");

            for (Instruction &inst : F.getEntryBlock()) {
                if (isStackAlign(inst, R_ESP)) {
                    DBG("remove stack align instruction (assumes R_ESP is set to a "
                        "16-byte "
                        "aligned address)");
                    inst.replaceAllUsesWith(inst.getOperand(0));
                    inst.eraseFromParent();
                    Changed = true;
                    break;
                }
            }
        }
        return Changed ? PreservedAnalyses::allInSet<CFGAnalyses>() : PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addUnalignStackPass(ModulePassManager &mpm) { mpm.addPass(UnalignStackPass()); }
