#ifndef _INITIALIZE_GLOBALS_H
#define _INITIALIZE_GLOBALS_H

#include "llvm/Pass.h"

using namespace llvm;

struct InternalizeGlobals : public ModulePass {
    static char ID;
    InternalizeGlobals() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _INITIALIZE_GLOBALS_H
