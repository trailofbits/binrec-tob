#include "rename_env.hpp"
#include "error.hpp"
#include "pass_utils.hpp"

#define PASS_NAME "rename_env"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;

static constexpr std::array<const char *, 8> Reg_Names =
    {"R_EAX", "R_ECX", "R_EDX", "R_EBX", "R_ESP", "R_EBP", "R_ESI", "R_EDI"};

template <typename T> static void replace_gep(Module &m, T *gep, Type *reg_ty)
{
    PASS_ASSERT(gep->getNumIndices() == 2);
    PASS_ASSERT(gep->hasAllConstantIndices());
    unsigned index = cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
    PASS_ASSERT(index < Reg_Names.size());
    gep->replaceAllUsesWith(cast<GlobalVariable>(m.getOrInsertGlobal(Reg_Names.at(index), reg_ty)));
}

// NOLINTNEXTLINE
auto RenameEnvPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    if (auto eip = m.getNamedGlobal("eip"))
        eip->setName("PC");

    if (GlobalVariable *regs = m.getNamedGlobal("regs")) {
        Type *reg_ty = cast<ArrayType>(regs->getType()->getElementType())->getElementType();
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
