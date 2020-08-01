#ifndef _ADDMEMARRAY_H
#define _ADDMEMARRAY_H

#include "llvm/Pass.h"
#include "llvm/IR/GlobalVariable.h"

#define MEMNAME "memory"
#define MEMSIZE 0xffffffff

using namespace llvm;

struct AddMemArrayPass : public BasicBlockPass {
    static char ID;
    AddMemArrayPass() : BasicBlockPass(ID) {}

    virtual bool doInitialization(Module &m);
    virtual bool runOnBasicBlock(BasicBlock &bb);

private:
    GlobalVariable *memory;
};

#endif  // _ADDMEMARRAY_H
