#ifndef _COUNT_EDGES_H
#define _COUNT_EDGES_H

#include "llvm/Pass.h"

using namespace llvm;

struct CountEdges : public ModulePass {
    static char ID;
    CountEdges() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _COUNT_EDGES_H
