#ifndef _COUNT_BLOCKS_H
#define _COUNT_BLOCKS_H

#include "llvm/Pass.h"

using namespace llvm;

struct CountBlocks : public ModulePass {
    static char ID;
    CountBlocks() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _COUNT_BLOCKS_H
