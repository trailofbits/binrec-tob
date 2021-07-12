#ifndef BINREC_MYDOTCFG_H
#define BINREC_MYDOTCFG_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct MyDotCfg : public ModulePass {
    static char ID;
    MyDotCfg() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_MYDOTCFG_H
