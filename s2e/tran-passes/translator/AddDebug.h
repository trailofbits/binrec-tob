#ifndef _ADDDEBUG_H
#define _ADDDEBUG_H

#include "llvm/Pass.h"

using namespace llvm;

struct AddDebug : public ModulePass {
    static char ID;
    AddDebug() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _ADDDEBUG_H
