#ifndef _PRUNENULLSUCCS_H
#define _PRUNENULLSUCCS_H

#include "llvm/Pass.h"

using namespace llvm;

struct PruneNullSuccs : public ModulePass {
    static char ID;
    PruneNullSuccs() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _PRUNENULLSUCCS_H
