#ifndef _REMOVEQEMUHELPERS_H
#define _REMOVEQEMUHELPERS_H

#include "llvm/Pass.h"

using namespace llvm;

struct RemoveQemuHelpers : public ModulePass {
    static char ID;
    RemoveQemuHelpers() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _REMOVEQEMUHELPERS_H
