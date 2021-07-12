#ifndef BINREC_COUNTEDGES_H
#define BINREC_COUNTEDGES_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct CountEdges : public ModulePass {
    static char ID;
    CountEdges() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_COUNTEDGES_H
