#ifndef _PCJUMPS_H
#define _PCJUMPS_H

#include "llvm/Pass.h"

using namespace llvm;

struct PcJumpsPass : public ModulePass {
    static char ID;
    PcJumpsPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _PCJUMPS_H
