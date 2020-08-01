#ifndef _REMOVE_CALLS_TO_EXCEPTION_H
#define _REMOVE_CALLS_TO_EXCEPTION_H

#include "llvm/Pass.h"

using namespace llvm;

struct RemoveCallsToException : public FunctionPass {
    static char ID;
    RemoveCallsToException() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &F);
};

#endif

