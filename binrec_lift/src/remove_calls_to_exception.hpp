#ifndef BINREC_REMOVE_CALLS_TO_EXCEPTION_HPP
#define BINREC_REMOVE_CALLS_TO_EXCEPTION_HPP

#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

using namespace llvm;

struct RemoveCallsToException : public FunctionPass {
    static char ID;
    RemoveCallsToException() : FunctionPass(ID) {}

    auto runOnFunction(Function &F) -> bool override;
};

#endif
