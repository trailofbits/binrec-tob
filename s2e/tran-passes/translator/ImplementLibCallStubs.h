#ifndef _IMPLEMENT_LIB_CALL_STUBS_H
#define _IMPLEMENT_LIB_CALL_STUBS_H

#include "llvm/Pass.h"

using namespace llvm;

struct ImplementLibCallStubs : public ModulePass {
    static char ID;
    ImplementLibCallStubs() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _IMPLEMENT_LIB_CALL_STUBS_H
