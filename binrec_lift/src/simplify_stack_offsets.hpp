#ifndef BINREC_SIMPLIFY_STACK_OFFSETS_HPP
#define BINREC_SIMPLIFY_STACK_OFFSETS_HPP

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct SimplifyStackOffsets : public ModulePass {
    static char ID;
    SimplifyStackOffsets() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;
};

#endif
