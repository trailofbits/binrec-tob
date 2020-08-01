#ifndef _INITIALIZE_FUNCTIONS_H
#define _INITIALIZE_FUNCTIONS_H

#include "llvm/Pass.h"

using namespace llvm;

struct AddAttributes : public FunctionPass {
    static char ID;
    AddAttributes() : FunctionPass(ID) {}

    virtual bool runOnFunction(Function &g);
};

#endif  // _INITIALIZE_FUNCTIONS_H
