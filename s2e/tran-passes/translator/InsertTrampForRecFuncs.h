#ifndef INSERT_TRAMP_FOR_REC_FUNCS_PASS_H
#define INSERT_TRAMP_FOR_REC_FUNCS_PASS_H

#include "llvm/Pass.h"
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>

using namespace llvm;

struct InsertTrampForRecFuncsPass : public ModulePass {
    struct FuncData {
        uint32_t origCallTargetAddress; //32 bit ptr
        std::unordered_set<uint32_t> origRetAddresses;
        StoreInst *recovCallTargetAddress = NULL; //instr when store origCallTargetAddress to "PC"
        std::unordered_set<StoreInst*> recovRetAddresses; //instr when store origRetAddress to "PC", go back to trampoline after this
    };
    
    static char ID;
    InsertTrampForRecFuncsPass() : ModulePass(ID) {}
    void getCBEntryAndReturns(Module &M, std::unordered_set<BasicBlock*> &returns, std::unordered_set<BasicBlock*> &entries);
    void loadFunctionMetadata();

    virtual bool runOnModule(Module &m);

private:
    
    std::map<uint32_t, std::set<uint32_t> > entryPcToReturnPcs;
    std::map<uint32_t, std::set<uint32_t> > entryPcToCallerPcs;
    std::map<uint32_t, uint32_t > callerPcToFollowUpPc;
    std::unordered_set<uint32_t> callers;
    
    std::map<uint32_t, std::set<uint32_t> > entryPcToBBPcs;
    std::map<uint32_t, std::set<uint32_t> > BBPcToEntryPcs;
    GlobalVariable *prevR_ESP;

};

#endif //INSERT_TRAMP_FOR_REC_FUNCS_PASS_H
