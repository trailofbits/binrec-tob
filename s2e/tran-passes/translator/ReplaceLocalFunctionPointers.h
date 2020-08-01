#ifndef _RDS_H
#define _RDS_H

#include "llvm/Pass.h"

#include <map>
using namespace llvm;

struct ReplaceLocalFunctionPointers : public FunctionPass {
    static char ID;
    ReplaceLocalFunctionPointers() : FunctionPass(ID) {}

    virtual bool doInitialization(Module &m);
    virtual bool runOnFunction(Function &f);

private:
    std::map<unsigned, BasicBlock*> allOriginalFunctions;

};

#endif  // _RDS_H
