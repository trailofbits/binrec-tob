#ifndef _PRUNE_LIBARGS_PUSH_H
#define _PRUNE_LIBARGS_PUSH_H

#include "llvm/Pass.h"

using namespace llvm;

struct PruneLibargsPush : public ModulePass {
    static char ID;
    PruneLibargsPush() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _PRUNE_LIBARGS_PUSH_H
