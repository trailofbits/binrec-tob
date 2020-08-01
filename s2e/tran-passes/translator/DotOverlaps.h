#ifndef _DOT_OVERLAPS_H
#define _DOT_OVERLAPS_H

#include "llvm/Pass.h"

using namespace llvm;

struct DotOverlaps : public ModulePass {
    static char ID;
    DotOverlaps() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _DOT_OVERLAPS_H
