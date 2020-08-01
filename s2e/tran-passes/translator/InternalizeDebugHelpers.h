#ifndef _INITIALIZE_DEBUG_HELPERS_H
#define _INITIALIZE_DEBUG_HELPERS_H

#include "llvm/Pass.h"

using namespace llvm;

struct InternalizeDebugHelpers : public FunctionPass {
    static char ID;
    InternalizeDebugHelpers() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &g);
};

#endif  // _INITIALIZE_DEBUG_HELPERS_H
