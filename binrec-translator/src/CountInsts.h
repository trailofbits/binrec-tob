#ifndef BINREC_COUNTINSTS_H
#define BINREC_COUNTINSTS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct CountInsts : public ModulePass {
    static char ID;
    CountInsts() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_COUNTINSTS_H
