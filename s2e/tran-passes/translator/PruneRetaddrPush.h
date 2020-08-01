#ifndef _PRUNE_RETADDR_PUSH_H
#define _PRUNE_RETADDR_PUSH_H

#include "llvm/Pass.h"

using namespace llvm;

struct PruneRetaddrPush : public ModulePass {
    static char ID;
    PruneRetaddrPush() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _PRUNE_RETADDR_PUSH_H
