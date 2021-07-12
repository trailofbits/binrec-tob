#ifndef BINREC_CONSTANTLOADS_H
#define BINREC_CONSTANTLOADS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct ConstantLoads : public ModulePass {
    static char ID;
    ConstantLoads() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_CONSTANTLOADS_H
