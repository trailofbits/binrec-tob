#ifndef _COUNT_PC_INSTS_H
#define _COUNT_PC_INSTS_H

#include "llvm/Pass.h"

using namespace llvm;

struct CountPcInsts : public ModulePass {
    static char ID;
    CountPcInsts() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif
