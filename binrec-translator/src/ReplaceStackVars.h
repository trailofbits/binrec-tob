#ifndef BINREC_REPLACESTACKVARS_H
#define BINREC_REPLACESTACKVARS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct ReplaceStackVars : public ModulePass {
    static char ID;
    ReplaceStackVars() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_REPLACESTACKVARS_H
