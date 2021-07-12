#ifndef BINREC_PESECTIONS_H
#define BINREC_PESECTIONS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct PESections : public ModulePass {
    static char ID;
    PESections() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_PESECTIONS_H
