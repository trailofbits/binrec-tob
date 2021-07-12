#ifndef BINREC_REMOVELIBCSTART_H
#define BINREC_REMOVELIBCSTART_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct RemoveLibcStart : public ModulePass {
    static char ID;
    RemoveLibcStart() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_REMOVELIBCSTART_H
