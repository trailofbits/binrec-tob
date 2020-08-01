#ifndef _REMOVE_BLOCKS_H
#define _REMOVE_BLOCKS_H

#include "llvm/Pass.h"

using namespace llvm;

struct RemoveBlocks : public ModulePass {
    static char ID;
    RemoveBlocks() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _REMOVE_BLOCKS_H
