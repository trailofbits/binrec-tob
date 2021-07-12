#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
constexpr std::array<const char *, 8> regNames = {"R_EAX", "R_ECX", "R_EDX", "R_EBX",
                                                  "R_ESP", "R_EBP", "R_ESI", "R_EDI"};

template <typename T> static void replaceGEP(Module &m, T *gep, Type *regTy) {
    assert(gep->getNumIndices() == 2);
    assert(gep->hasAllConstantIndices());
    unsigned index = cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
    assert(index < regNames.size());
    gep->replaceAllUsesWith(cast<GlobalVariable>(m.getOrInsertGlobal(regNames.at(index), regTy)));
}

/// S2E Rename globals from env struct to match what the next passes expect
class RenameEnvPass : public PassInfoMixin<RenameEnvPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        m.getNamedGlobal("eip")->setName("PC");

        GlobalVariable *regs = m.getNamedGlobal("regs");
        Type *regTy = cast<ArrayType>(regs->getType()->getElementType())->getElementType();
        for (User *use : regs->users()) {
            if (auto *gep = dyn_cast<GetElementPtrInst>(use))
                replaceGEP(m, gep, regTy);
        }
        for (User *use : regs->users()) {
            auto *gep = cast<GEPOperator>(use);
            replaceGEP(m, gep, regTy);
        }

        return PreservedAnalyses::allInSet<CFGAnalyses>();
    }
};
} // namespace

void binrec::addRenameEnvPass(llvm::ModulePassManager &mpm) { mpm.addPass(RenameEnvPass()); }
