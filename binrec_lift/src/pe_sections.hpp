#ifndef BINREC_PE_SECTIONS_HPP
#define BINREC_PE_SECTIONS_HPP

#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

using namespace llvm;

struct PESections : public ModulePass {
    static char ID;
    PESections() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif
