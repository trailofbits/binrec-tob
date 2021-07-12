#ifndef BINREC_COUNTPCINSTS_H
#define BINREC_COUNTPCINSTS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct CountPcInsts : public ModulePass {
    static char ID;
    CountPcInsts() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif
