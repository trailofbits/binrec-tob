#ifndef _SIMPLIFY_STACK_OFFSETS_H
#define _SIMPLIFY_STACK_OFFSETS_H

#include "llvm/Pass.h"

using namespace llvm;

struct SimplifyStackOffsets : public ModulePass {
    static char ID;
    SimplifyStackOffsets() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _SIMPLIFY_STACK_OFFSETS_H
