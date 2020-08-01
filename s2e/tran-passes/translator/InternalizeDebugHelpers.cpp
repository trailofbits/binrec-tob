#include "InternalizeDebugHelpers.h"
#include "PassUtils.h"

using namespace llvm;

char InternalizeDebugHelpers::ID = 0;
static RegisterPass<InternalizeDebugHelpers> X("internalize-debug-helpers",
        "S2E Internalize debug helper functions for removal by -globalde", false, false);

bool InternalizeDebugHelpers::runOnFunction(Function &f) {
    if (f.hasName() && (f.getName().startswith("mywrite") ||
                f.getName().startswith("helper_"))) {
        f.setLinkage(GlobalValue::InternalLinkage);
        return true;
    }

    return false;
}
