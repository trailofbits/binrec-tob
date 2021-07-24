#include "internalize_globals.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto InternalizeGlobalsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    for (GlobalVariable &g : m.globals()) {
        if (g.hasInitializer()) {
            g.setLinkage(GlobalValue::InternalLinkage);
            continue;
        }

        Type *ty = g.getType()->getElementType();
        Constant *init = nullptr;

        if (ty->isPointerTy())
            init = ConstantPointerNull::get(cast<PointerType>(ty));
        else if (ty->isAggregateType())
            init = ConstantAggregateZero::get(ty);
        else if (ty->isIntegerTy())
            init = ConstantInt::get(ty, 0);

        failUnless(init != nullptr, "unsupported global type");

        g.setInitializer(init);
        g.setLinkage(GlobalValue::InternalLinkage);
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
