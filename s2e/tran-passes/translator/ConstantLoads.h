#ifndef _CONSTANT_LOADS_H
#define _CONSTANT_LOADS_H

#include "llvm/Pass.h"

using namespace llvm;

struct ConstantLoads : public ModulePass {
    static char ID;
    ConstantLoads() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _CONSTANT_LOADS_H
