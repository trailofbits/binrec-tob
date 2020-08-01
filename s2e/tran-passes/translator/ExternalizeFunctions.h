#ifndef _EXTERNALIZE_FUNCTIONS_H
#define _EXTERNALIZE_FUNCTIONS_H

#include "llvm/Pass.h"

using namespace llvm;

struct ExternalizeFunctions : public ModulePass {
    static char ID;
    ExternalizeFunctions() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _EXTERNALIZE_FUNCTIONS_H
