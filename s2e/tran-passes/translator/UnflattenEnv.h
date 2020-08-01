#ifndef _UNFLATTEN_ENV_H
#define _UNFLATTEN_ENV_H

#include "PassUtils.h"

using namespace llvm;

struct UnflattenEnv : public FunctionPass {
    static char ID;
    UnflattenEnv() : FunctionPass(ID) {}

    virtual bool doInitialization(Module &m);
    virtual bool doFinalization(Module &m);
    virtual bool runOnFunction(Function &f);

private:
    GlobalVariable *env;
    StructType *envType;
    DataLayout *layout;
    const StructLayout *envLayout;

    unsigned getMemberIndex(CompositeType *ty, uint64_t offset);
};

#endif  // _UNFLATTEN_ENV_H
