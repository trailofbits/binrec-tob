#ifndef _ENV_TO_ALLOCAS_H
#define _ENV_TO_ALLOCAS_H

#include "llvm/Pass.h"

using namespace llvm;

struct EnvToAllocas : public ModulePass {
    static char ID;
    EnvToAllocas() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _ENV_TO_ALLOCAS_H
