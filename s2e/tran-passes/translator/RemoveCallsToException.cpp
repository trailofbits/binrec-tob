#include "RemoveCallsToException.h"
#include "PassUtils.h"
#include "DebugUtils.h"
#include <stdlib.h>
#include <stdio.h>

using namespace llvm;

char RemoveCallsToException::ID = 0;
static RegisterPass<RemoveCallsToException> X("remove-exceptions",
        "Remove calls to helper_raise_exception", false, false);

bool RemoveCallsToException::runOnFunction(Function &F) {

    std::vector<Instruction*> eraseList;
    bool changed = false;
    for (Function::iterator bb = F.begin(), bbe = F.end(); bb != bbe; ++bb) {
       BasicBlock &b = *bb;
       for (BasicBlock::iterator i = b.begin(), ie = b.end(); i != ie; ++i) {
          if (CallInst *call = dyn_cast<CallInst>(&*i)){
      
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


