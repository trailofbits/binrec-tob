#ifndef _CYCLOMATIC_COMPLEXITY_H
#define _CYCLOMATIC_COMPLEXITY_H

#include "llvm/Pass.h"

using namespace llvm;

struct CyclomaticComplexity : public ModulePass {
    static char ID;
    CyclomaticComplexity() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _CYCLOMATIC_COMPLEXITY_H
