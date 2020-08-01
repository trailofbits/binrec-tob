//
// Created by jn on 11/6/17.
//
#ifndef BINREC_RECOVFUNCTRAMPOLINES_H
#define BINREC_RECOVFUNCTRAMPOLINES_H

#include "llvm/Pass.h"
//#include "llvm/IR/InstIterator.h"

using namespace llvm;

struct RecovFuncTrampolinesPass : public ModulePass {
    static char ID;
    RecovFuncTrampolinesPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif //BINREC_RECOVFUNCTRAMPOLINES_H
