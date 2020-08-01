#include <iostream>
#include "ImplementLibCallsNewPLT.h"
#include "PassUtils.h"
#include <llvm/IR/Module.h>
using namespace llvm;

char ImplementLibCallsNewPLT::ID = 0;
static RegisterPass<ImplementLibCallsNewPLT> X("implement-lib-calls-new-plt",
        "make a stub call into a full fledged call through the plt",
        false, false);

bool ImplementLibCallsNewPLT::runOnModule(Module &m) {
    for (auto f = m.getFunctionList().begin(),  endFref = m.getFunctionList().end();
                f != endFref; ++f) { //need a non-const f ptr for foreach macro to work
        if (f->hasName() && f->getName().startswith("__stub")) {
            Constant* newFc = m.getOrInsertFunction(f->getName().drop_front(7),
                    f->getFunctionType(), f->getAttributes());
            Function* newF = dyn_cast<Function>(newFc);
            foreach_use_if(f, CallInst, call, {
                std::vector<Value*> args;
                for (auto it = call->arg_begin(); it != call->arg_end();++it){
                    args.push_back(dyn_cast<Value>(it));
                }
                IRBuilder<> b(call);
                Value* ret = b.CreateCall(newF, args);
                call->replaceAllUsesWith(ret);
                call->eraseFromParent();
            });
        }
    }

    return true;
}
