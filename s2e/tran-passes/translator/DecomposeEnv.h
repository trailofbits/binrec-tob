#ifndef _DECOMPOSE_ENV_H
#define _DECOMPOSE_ENV_H

#include "PassUtils.h"

using namespace llvm;

struct DecomposeEnv : public ModulePass {
    static char ID;
    DecomposeEnv() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);

private:
    std::vector<StringRef> names;
    StructType *envType;

    void getMemberNames(MDNode *info);
    GlobalVariable *getGlobal(Module &m, unsigned index);
    void processInst(Module &m, Instruction *inst);
};

#endif  // _DECOMPOSE_ENV_H
