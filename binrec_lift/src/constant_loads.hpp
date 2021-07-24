#ifndef BINREC_CONSTANT_LOADS_HPP
#define BINREC_CONSTANT_LOADS_HPP

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct ConstantLoads : public ModulePass {
    static char ID;
    ConstantLoads() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif
