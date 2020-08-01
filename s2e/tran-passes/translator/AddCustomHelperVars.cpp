#include "AddCustomHelperVars.h"
#include "PassUtils.h"

using namespace llvm;

char AddCustomHelperVars::ID = 0;
static RegisterPass<AddCustomHelperVars> X("add-custom-helper-vars",
        "S2E Insert variables that can be used by custom helpers",
        false, false);

bool AddCustomHelperVars::runOnModule(Module &m) {
    Type *ty = Type::getInt32Ty(m.getContext());
    new GlobalVariable(m, ty, true, GlobalValue::ExternalLinkage,
            ConstantInt::get(ty, debugVerbosity), "debug_verbosity");

    return true;
}
