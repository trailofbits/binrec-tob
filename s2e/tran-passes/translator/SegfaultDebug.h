#ifndef _SEGFAULT_DEBUG_H
#define _SEGFAULT_DEBUG_H

#include "llvm/Pass.h"

using namespace llvm;

struct SegfaultDebug : public FunctionPass {
    static char ID;
    SegfaultDebug() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &g);
};

#endif

