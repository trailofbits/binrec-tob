#ifndef _COUNT_INSTS_H
#define _COUNT_INSTS_H

#include "llvm/Pass.h"

using namespace llvm;

struct CountInsts : public ModulePass {
    static char ID;
    CountInsts() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _COUNT_INSTS_H
