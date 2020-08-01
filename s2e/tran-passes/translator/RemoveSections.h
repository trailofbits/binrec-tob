#ifndef _REMOVE_SECTIONS_H
#define _REMOVE_SECTIONS_H

#include "llvm/Pass.h"

using namespace llvm;

struct RemoveSections : public ModulePass {
    static char ID;
    RemoveSections() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _REMOVE_SECTIONS_H
