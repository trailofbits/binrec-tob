#ifndef _UNALIAS_GLOBALS_H
#define _UNALIAS_GLOBALS_H

#include "llvm/Pass.h"
#include "llvm/IR/GlobalVariable.h"

using namespace llvm;

struct UnaliasGlobals : public BasicBlockPass {
    static char ID;
    UnaliasGlobals() : BasicBlockPass(ID) {}

    virtual bool doInitialization(Module &m);
    virtual bool runOnBasicBlock(BasicBlock &bb);

private:
    MDNode *registerScope, *memoryScope;
    GlobalVariable *memory;
};

#endif  // _UNALIAS_GLOBALS_H
