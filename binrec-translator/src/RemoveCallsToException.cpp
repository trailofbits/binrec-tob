#include "RemoveCallsToException.h"
#include "DebugUtils.h"
#include "PassUtils.h"
#include <cstdio>
#include <cstdlib>

using namespace llvm;

char RemoveCallsToException::ID = 0;
static RegisterPass<RemoveCallsToException> X("remove-exceptions", "Remove calls to helper_raise_exception", false,
                                              false);

auto RemoveCallsToException::runOnFunction(Function &F) -> bool {

    std::vector<Instruction *> eraseList;
    bool changed = false;
    for (auto &b : F) {
        for (auto &i : b) {
            if (auto *call = dyn_cast<CallInst>(&i)) {

                Function *f = call->getCalledFunction();

                if (f && f->getName() == "helper_raise_exception") {
                    eraseList.push_back(call);
                    changed = true;
                }
            }
        }
    }

    for (Instruction *i : eraseList)
        i->eraseFromParent();

    return changed;
}
