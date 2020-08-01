#ifndef _PEMAIN_H
#define _PEMAIN_H

#include "llvm/Pass.h"

using namespace llvm;

struct PEMainPass : public ModulePass {
    static char ID;
    PEMainPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _PEMAIN_H
