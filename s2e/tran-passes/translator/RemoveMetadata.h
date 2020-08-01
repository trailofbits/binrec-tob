#ifndef _REMOVEMETADATA_H
#define _REMOVEMETADATA_H

#include "llvm/Pass.h"

using namespace llvm;

struct RemoveMetadata : public ModulePass {
    static char ID;
    RemoveMetadata() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _REMOVEMETADATA_H
