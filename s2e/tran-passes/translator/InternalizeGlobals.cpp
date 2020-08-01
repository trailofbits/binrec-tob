#include "InternalizeGlobals.h"
#include "PassUtils.h"

using namespace llvm;

char InternalizeGlobals::ID = 0;
static RegisterPass<InternalizeGlobals> X("internalize-globals",
        "S2E Internalize and zero-initialize global variables", false, false);

bool InternalizeGlobals::runOnModule(Module &m) {
    foreach2(g, m.global_begin(), m.global_end()) {
        if (g->hasInitializer()) {
            g->setLinkage(GlobalValue::InternalLinkage);
            continue;
        }

        Type *ty = g->getType()->getElementType();
        Constant *init;

        if (ty->isPointerTy())
            init = ConstantPointerNull::get(cast<PointerType>(ty));
        else if (ty->isAggregateType())
            init = ConstantAggregateZero::get(ty);
        else if (ty->isIntegerTy())
            init = ConstantInt::get(ty, 0);
        else
            failUnless(false, "unsupported global type");

        g->setInitializer(init);
        g->setLinkage(GlobalValue::InternalLinkage);
    }

    return true;
}
