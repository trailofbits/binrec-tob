#ifndef BINREC_PEMAIN_H
#define BINREC_PEMAIN_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct PEMainPass : public ModulePass {
    static char ID;
    PEMainPass() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_PEMAIN_H
