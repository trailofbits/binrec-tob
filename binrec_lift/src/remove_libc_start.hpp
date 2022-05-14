#ifndef BINREC_REMOVE_LIBC_START_HPP
#define BINREC_REMOVE_LIBC_START_HPP

#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

using namespace llvm;

struct RemoveLibcStart : public ModulePass {
    static char ID;
    RemoveLibcStart() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif
