#ifndef BINREC_SIMPLIFYSTACKOFFSETS_H
#define BINREC_SIMPLIFYSTACKOFFSETS_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct SimplifyStackOffsets : public ModulePass {
    static char ID;
    SimplifyStackOffsets() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_SIMPLIFYSTACKOFFSETS_H
