#ifndef _INTERNALIZE_IMPORTS_H
#define _INTERNALIZE_IMPORTS_H

#include "llvm/Pass.h"

using namespace llvm;

struct InternalizeImports : public FunctionPass {
    static char ID;
    InternalizeImports() : FunctionPass(ID) {}

    virtual bool doInitialization(Module &m);
    virtual bool runOnFunction(Function &f);
};

#endif  // _INTERNALIZE_IMPORTS_H
