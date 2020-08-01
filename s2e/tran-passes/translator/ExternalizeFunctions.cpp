#include "ExternalizeFunctions.h"
#include "PassUtils.h"

using namespace llvm;

char ExternalizeFunctions::ID = 0;
static RegisterPass<ExternalizeFunctions> X("externalize-functions",
        "S2E Give all function definitions and declarations external linkage",
        false, false);

bool ExternalizeFunctions::runOnModule(Module &m) {
    for (Function &f : m)
        f.setLinkage(GlobalValue::ExternalLinkage);

    return true;
}
