#ifndef BINREC_REMOVECALLSTOEXCEPTION_H
#define BINREC_REMOVECALLSTOEXCEPTION_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct RemoveCallsToException : public FunctionPass {
    static char ID;
    RemoveCallsToException() : FunctionPass(ID) {}

    auto runOnFunction(Function &F) -> bool override;
};

#endif
