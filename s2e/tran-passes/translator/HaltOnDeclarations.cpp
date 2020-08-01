#include <algorithm>
#include "llvm/IR/InlineAsm.h"

#include "HaltOnDeclarations.h"
#include "PassUtils.h"
#include "DebugUtils.h"

using namespace llvm;

char HaltOnDeclarationsPass::ID = 0;
static RegisterPass<HaltOnDeclarationsPass> X("halt-on-declarations",
        "S2E List any function declarations whose implementation is missing",
        false, false);

bool HaltOnDeclarationsPass::runOnModule(Module &m) {
    bool halt = false;

    std::vector<Function*> intrinsics;

    for (Function &f : m) {
        if (f.empty() && f.getNumUses() > 0 &&
                f.hasName() && !f.getName().startswith("__stub")) {
            if (f.isIntrinsic()) {
                WARNING("intrinsic " << f.getName() <<
                        " may create PLT entries (used " <<
                        f.getNumUses() << " times)");
                intrinsics.push_back(&f);
            }
            //else {
                //ERROR("missing implementation for " << f.getName() <<
                        //" (used " << f.getNumUses() << " times)");
                //halt = true;
            //}
        }
    }

    if (halt) {
        errs() << "found function declarations, exiting with error status\n";
        exit(1);
    }

    return false;
}
