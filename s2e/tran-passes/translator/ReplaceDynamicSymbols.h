#ifndef _RDS_H
#define _RDS_H

#include "llvm/Pass.h"

#include <map>
using namespace llvm;

struct ReplaceDynamicSymbols : public FunctionPass {
    static char ID;
    ReplaceDynamicSymbols() : FunctionPass(ID) {}

    virtual bool doInitialization(Module &m);
    virtual bool runOnFunction(Function &f);

private:
    std::map<int32_t, std::string> symmap;

};

#endif  // _RDS_H
