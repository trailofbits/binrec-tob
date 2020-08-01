#ifndef _ELFSYMBOLS_H
#define _ELFSYMBOLS_H

#include "llvm/Pass.h"

using namespace llvm;

struct ELFSymbolsPass : public ModulePass {
    static char ID;
    ELFSymbolsPass() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _ELFSYMBOLS_H
