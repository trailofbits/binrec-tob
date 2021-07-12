#ifndef BINREC_REMOVEBLOCKS_H
#define BINREC_REMOVEBLOCKS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct RemoveBlocks : public ModulePass {
    static char ID;
    RemoveBlocks() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_REMOVEBLOCKS_H
