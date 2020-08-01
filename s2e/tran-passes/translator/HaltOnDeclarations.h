#ifndef _HALTONDECLARATIONS_H
#define _HALTONDECLARATIONS_H

#include "llvm/Pass.h"

using namespace llvm;

struct HaltOnDeclarationsPass : public ModulePass {
    static char ID;
    HaltOnDeclarationsPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _HALTONDECLARATIONS_H
