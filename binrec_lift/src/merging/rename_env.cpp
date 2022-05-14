#include "rename_env.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;

static constexpr std::array<const char *, 8> Reg_Names =
    {"R_EAX", "R_ECX", "R_EDX", "R_EBX", "R_ESP", "R_EBP", "R_ESI", "R_EDI"};

template <typename T> static void replace_gep(Module &m, T *gep, Type *reg_ty)
{
    assert(gep->getNumIndices() == 2);
    assert(gep->hasAllConstantIndices());
    unsigned index = cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
    assert(index < Reg_Names.size());
    gep->replaceAllUsesWith(cast<GlobalVariable>(m.getOrInsertGlobal(Reg_Names.at(index), reg_ty)));
}

// NOLINTNEXTLINE
auto RenameEnvPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    if (auto eip = m.getNamedGlobal("eip"))
        eip->setName("PC");

    if (GlobalVariable *regs = m.getNamedGlobal("regs")) {
        Type *reg_ty = cast<ArrayType>(regs->getType()->getPointerElementType())->getPointerElementType();
        for (User *use : regs->users()) {
            if (auto *gep = dyn_cast<GetElementPtrInst>(use))
                replace_gep(m, gep, reg_ty);
        }
        for (User *use : regs->users()) {
            auto *gep = cast<GEPOperator>(use);
            replace_gep(m, gep, reg_ty);
        }
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
