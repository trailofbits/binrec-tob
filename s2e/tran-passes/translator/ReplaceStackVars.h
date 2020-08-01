#ifndef _REPLACE_STACK_VARS_H
#define _REPLACE_STACK_VARS_H

#include "llvm/Pass.h"

using namespace llvm;

struct ReplaceStackVars : public ModulePass {
    static char ID;
    ReplaceStackVars() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _REPLACE_STACK_VARS_H
