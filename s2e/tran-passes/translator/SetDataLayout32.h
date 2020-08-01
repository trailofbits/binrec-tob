#ifndef _SET_DATA_LAYOUT_32
#define _SET_DATA_LAYOUT_32

#include "llvm/Pass.h"

using namespace llvm;

struct SetDataLayout32 : public ModulePass {
    static char ID;
    SetDataLayout32() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _SET_DATA_LAYOUT_32
