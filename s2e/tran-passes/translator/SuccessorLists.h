#ifndef _SUCCESSOR_LISTS_H
#define _SUCCESSOR_LISTS_H

#include <map>
#include "llvm/Pass.h"

using namespace llvm;

struct SuccessorLists : public ModulePass {
    static char ID;
    SuccessorLists() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);

private:
    std::map<uint32_t, Function*> bbCache;
    Function *getCachedFunc(Module &m, uint32_t pc);
};

#endif  // _SUCCESSOR_LISTS_H
