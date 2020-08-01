#ifndef _UNALIGN_STACK_H
#define _UNALIGN_STACK_H

#include "llvm/Pass.h"

using namespace llvm;

struct UnalignStack : public ModulePass {
    static char ID;
    UnalignStack() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _UNALIGN_STACK_H
