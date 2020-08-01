#ifndef _EXTERNALCALLS_H
#define _EXTERNALCALLS_H

#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/GlobalVariable.h"

using namespace llvm;

struct ExternPLTPass : public BasicBlockPass {
    static char ID;
    ExternPLTPass() : BasicBlockPass(ID) {}

    virtual bool doInitialization(Module &m);
    virtual bool runOnBasicBlock(BasicBlock &bb);

private:
    Function *helper;
    Function *nonlib_setjmp;
    Function *nonlib_longjmp;
};

#endif  // _EXTERNALCALLS_H
