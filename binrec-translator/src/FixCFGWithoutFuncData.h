#ifndef BINREC_FIXCFGWITHOUTFUNCDATA_H
#define BINREC_FIXCFGWITHOUTFUNCDATA_H

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <map>
#include <unordered_set>

using namespace llvm;

struct FixCFGWithoutFuncData : public ModulePass {
    static char ID;
    FixCFGWithoutFuncData() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;

private:
    void getBBs(Function &F, std::map<unsigned, BasicBlock *> &BBMap);
    void getBBsWithLibCalls(Function &F, std::unordered_set<BasicBlock *> &BBSet);
};

#endif
