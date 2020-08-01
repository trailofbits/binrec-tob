#ifndef _PRUNE_TRIVIALLY_DEAD_SUCCS_H
#define _PRUNE_TRIVIALLY_DEAD_SUCCS_H

#include "llvm/Pass.h"

using namespace llvm;

struct PruneTriviallyDeadSuccs : public ModulePass {
    static char ID;
    PruneTriviallyDeadSuccs() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _PRUNE_TRIVIALLY_DEAD_SUCCS_H
