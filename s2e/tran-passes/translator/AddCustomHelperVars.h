#ifndef _ADD_CUSTOM_HELPER_VARS_H
#define _ADD_CUSTOM_HELPER_VARS_H

#include "llvm/Pass.h"

using namespace llvm;

struct AddCustomHelperVars : public ModulePass {
    static char ID;
    AddCustomHelperVars() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _ADD_CUSTOM_HELPER_VARS_H
