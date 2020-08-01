#ifndef _REMOVEMEMARRAY_H
#define _REMOVEMEMARRAY_H

#include "llvm/Pass.h"

using namespace llvm;

struct RemoveMemArrayPass : public ModulePass {
    static char ID;
    RemoveMemArrayPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _REMOVEMEMARRAY_H
