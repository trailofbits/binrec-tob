#ifndef _DETECT_VARS_H
#define _DETECT_VARS_H

#include "llvm/Pass.h"

using namespace llvm;

struct DetectVars : public ModulePass {
    static char ID;
    DetectVars() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _DETECT_VARS_H
