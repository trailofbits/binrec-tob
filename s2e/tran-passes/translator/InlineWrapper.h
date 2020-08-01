#ifndef _INLINE_WRAPPER_H
#define _INLINE_WRAPPER_H

#include "llvm/Pass.h"

using namespace llvm;

struct InlineWrapper : public ModulePass {
    static char ID;
    InlineWrapper() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _INLINE_WRAPPER_H
