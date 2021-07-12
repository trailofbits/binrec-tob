#ifndef BINREC_CYCLOMATICCOMPLEXITY_H
#define BINREC_CYCLOMATICCOMPLEXITY_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct CyclomaticComplexity : public ModulePass {
    static char ID;
    CyclomaticComplexity() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_CYCLOMATICCOMPLEXITY_H
