#ifndef BINREC_DETECTVARS_H
#define BINREC_DETECTVARS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct DetectVars : public ModulePass {
    static char ID;
    DetectVars() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_DETECTVARS_H
