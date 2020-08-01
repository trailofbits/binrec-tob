#ifndef _RENAME_BLOCK_FUNCS_H
#define _RENAME_BLOCK_FUNCS_H

#include "llvm/Pass.h"

using namespace llvm;

struct RenameBlockFuncs : public ModulePass {
    static char ID;
    RenameBlockFuncs() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _RENAME_BLOCK_FUNCS_H
