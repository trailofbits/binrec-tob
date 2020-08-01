#ifndef _DOT_MERGED_BLOCKS_H
#define _DOT_MERGED_BLOCKS_H

#include "llvm/Pass.h"

using namespace llvm;

struct DotMergedBlocks : public ModulePass {
    static char ID;
    DotMergedBlocks() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _DOT_MERGED_BLOCKS_H
