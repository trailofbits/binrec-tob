#ifndef _RENAME_ENV_H
#define _RENAME_ENV_H

#include "llvm/Pass.h"

using namespace llvm;

struct RenameEnv : public ModulePass {
    static char ID;
    RenameEnv() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _RENAME_ENV_H
