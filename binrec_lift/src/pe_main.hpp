#ifndef BINREC_PE_MAIN_HPP
#define BINREC_PE_MAIN_HPP

#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

using namespace llvm;

struct PEMainPass : public ModulePass {
    static char ID;
    PEMainPass() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif
