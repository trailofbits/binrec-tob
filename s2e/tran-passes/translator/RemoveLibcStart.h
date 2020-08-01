#ifndef _REMOVELIBCSTART_H
#define _REMOVELIBCSTART_H

#include "llvm/Pass.h"

using namespace llvm;

struct RemoveLibcStart : public ModulePass {
    static char ID;
    RemoveLibcStart() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _REMOVELIBCSTART_H
