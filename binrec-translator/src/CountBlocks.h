#ifndef BINREC_COUNTBLOCKS_H
#define BINREC_COUNTBLOCKS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct CountBlocks : public ModulePass {
    static char ID;
    CountBlocks() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_COUNTBLOCKS_H
