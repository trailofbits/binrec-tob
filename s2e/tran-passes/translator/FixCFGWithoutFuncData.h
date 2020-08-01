#ifndef _FIX_CFG_WITHOUT_FUNC_DATA_H
#define _FIX_CFG_WITHOUT_FUNC_DATA_H

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include <unordered_set>
#include <map>

using namespace llvm;

struct FixCFGWithoutFuncData : public ModulePass {
    static char ID;
    FixCFGWithoutFuncData() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);

private:
    void getBBs(Function &F, std::map<unsigned, BasicBlock*> &BBMap);
    void getBBsWithLibCalls(Function &F, std::unordered_set<BasicBlock*> &BBSet); 
};

#endif 
