#include "InternalizeFunctions.h"
#include "PassUtils.h"

using namespace llvm;

char InternalizeFunctions::ID = 0;
static RegisterPass<InternalizeFunctions> X("internalize-functions",
        "S2E Internalize functions for removal by -globalde", false, false);

bool InternalizeFunctions::runOnFunction(Function &f) {
    if (f.hasName() && f.getName() != "main" &&
            !f.getName().startswith("mywrite") &&
            !f.getName().startswith("__stub")) {
        f.setLinkage(GlobalValue::InternalLinkage);
        return true;
    }

    return false;
}
