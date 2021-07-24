#ifndef BINREC_REMOVE_LIBC_START_HPP
#define BINREC_REMOVE_LIBC_START_HPP

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct RemoveLibcStart : public ModulePass {
    static char ID;
    RemoveLibcStart() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif
