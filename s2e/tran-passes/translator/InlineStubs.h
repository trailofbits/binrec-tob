#ifndef _INLINESTUBS_H
#define _INLINESTUBS_H

#include "llvm/Pass.h"

using namespace llvm;

struct InlineStubsPass : public ModulePass {
    static char ID;
    InlineStubsPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _INLINESTUBS_H
