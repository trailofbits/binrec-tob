#ifndef BINREC_REPLACELOCALFUNCTIONPOINTERS_H
#define BINREC_REPLACELOCALFUNCTIONPOINTERS_H

#include <llvm/IR/PassManager.h>
#include <map>

using namespace llvm;

struct ReplaceLocalFunctionPointers : public FunctionPass {
    static char ID;
    ReplaceLocalFunctionPointers() : FunctionPass(ID) {}

    auto doInitialization(Module &m) -> bool override;
    auto runOnFunction(Function &f) -> bool override;

private:
    std::map<unsigned, BasicBlock *> allOriginalFunctions;
};

#endif // BINREC_REPLACELOCALFUNCTIONPOINTERS_H
