#include "InternalizeImports.h"
#include "PassUtils.h"

using namespace llvm;

char InternalizeImports::ID = 0;
static RegisterPass<InternalizeImports> x("internalize-imports",
        "S2E Internalize non-recovered functions for -globaldce", false, false);

bool InternalizeImports::doInitialization(Module &m) {
    m.setTargetTriple("i386-unknown-linux-gnu");
    return true;
}

bool InternalizeImports::runOnFunction(Function &f) {
    if (isRecoveredBlock(&f))
        return false;
   
    //Prevent -globaldce from removing it since we will call it from main function
    if(f.getName() == "helper_fninit")
        return false;

    f.setLinkage(GlobalValue::InternalLinkage);
    return true;
}
