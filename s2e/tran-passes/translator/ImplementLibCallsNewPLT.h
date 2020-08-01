#ifndef _IMPLEMENT_LIB_CALLS_NEW_PLT_H
#define _IMPLEMENT_LIB_CALLS_NEW_PLT_H

#include "llvm/Pass.h"

using namespace llvm;

struct ImplementLibCallsNewPLT : public ModulePass {
    static char ID;
    ImplementLibCallsNewPLT() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _IMPLEMENT_LIB_CALLS_NEW_PLT_H
