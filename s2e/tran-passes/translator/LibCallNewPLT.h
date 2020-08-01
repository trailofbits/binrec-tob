#ifndef _LIBCALLNEWPLT_H
#define _LIBCALLNEWPLT_H

#include <map>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;


struct LibCallNewPLT : public ModulePass {
    static char ID;
    LibCallNewPLT() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);

private:
    Module *module;
};

#endif  //_LIBCALLNEWPLT_H
