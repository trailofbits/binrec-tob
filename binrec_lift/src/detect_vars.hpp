#ifndef BINREC_DETECT_VARS_HPP
#define BINREC_DETECT_VARS_HPP

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct DetectVars : public ModulePass {
    static char ID;
    DetectVars() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif
