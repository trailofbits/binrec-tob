#ifndef _UNIMPLEMENT_CUSTOM_HELPERS_H
#define _UNIMPLEMENT_CUSTOM_HELPERS_H

#include "llvm/Pass.h"

using namespace llvm;

struct UnimplementCustomHelpers : public ModulePass {
    static char ID;
    UnimplementCustomHelpers() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _UNIMPLEMENT_CUSTOM_HELPERS_H
