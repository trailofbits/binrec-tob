#ifndef _MERGEFUNCTIONS_H
#define _MERGEFUNCTIONS_H

#include "llvm/Pass.h"

using namespace llvm;

struct MergeFunctionsPass : public ModulePass {
    static char ID;
    MergeFunctionsPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _MERGEFUNCTIONS_H
