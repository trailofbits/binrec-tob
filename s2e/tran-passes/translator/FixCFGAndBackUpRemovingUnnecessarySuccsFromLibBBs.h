#ifndef _FIX_CFG_LIB_H
#define _FIX_CFG_LIB_H

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"
#include <unordered_set>
#include <unordered_map>
#include <map>
#include <set>

using namespace llvm;

struct FixCFGLib : public ModulePass {
    static char ID;
    FixCFGLib() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);

private:
    
    std::map<uint32_t, std::set<uint32_t> > entryPcToReturnPcs;
    std::map<uint32_t, std::set<uint32_t> > entryPcToCallerPcs;
    std::map<uint32_t, uint32_t > callerPcToFollowUpPc;
    
    std::map<uint32_t, std::set<uint32_t> > entryPcToBBPcs;
    std::map<uint32_t, std::set<uint32_t> > BBPcToEntryPcs;
    std::unordered_set<BasicBlock *> libFuncsTakingCallback;
    std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *> > jmpBBs;

    bool hasPc(BasicBlock *bb, GlobalVariable *PC, uint32_t pc);
    void findCallerBBs(Function &F, std::unordered_map<unsigned, std::unordered_set<BasicBlock*> > &callerPcToCallerBB);
    void loadFunctionMetadata();
    void getBBs(Function &F, std::map<unsigned, BasicBlock*> &BBMap);
    void getBBsWithLibCalls(Function &F, std::unordered_set<BasicBlock*> &BBSet); 
    void addSwitchCases(std::map<unsigned, BasicBlock*> &BBMap,
                             std::unordered_map<BasicBlock *, std::unordered_set<unsigned> > &newSuccs);
    void optimizeEdges(std::unordered_set<BasicBlock*> &BBSet);
};

#endif 
