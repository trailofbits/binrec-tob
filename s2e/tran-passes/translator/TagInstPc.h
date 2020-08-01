#ifndef _TAG_INST_PC_H
#define _TAG_INST_PC_H

#include "llvm/Pass.h"

using namespace llvm;

struct TagInstPc : public ModulePass {
    static char ID;
    TagInstPc() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _TAG_INST_PC_H
