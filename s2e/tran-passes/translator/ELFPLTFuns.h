#ifndef _ELFPLTFUNS_H
#define _ELFPLTFUNS_H

#include "llvm/Pass.h"

using namespace llvm;

struct ELFPLTFunsPass : public ModulePass {
    static char ID;
    ELFPLTFunsPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _ELFPLTFUNS_H
