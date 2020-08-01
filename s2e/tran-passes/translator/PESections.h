#ifndef _PESECTIONS_H
#define _PESECTIONS_H

#include "llvm/Pass.h"

using namespace llvm;

struct PESections : public ModulePass {
    static char ID;
    PESections() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _PESECTIONS_H
