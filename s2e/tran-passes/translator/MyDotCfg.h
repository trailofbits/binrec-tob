#ifndef _MY_DOT_CFG_H
#define _MY_DOT_CFG_H

#include "llvm/Pass.h"

using namespace llvm;

struct MyDotCfg : public ModulePass {
    static char ID;
    MyDotCfg() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _MY_DOT_CFG_H
