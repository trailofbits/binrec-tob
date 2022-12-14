#ifndef BINREC_CUSTOM_LOOP_UNROLL_HPP
#define BINREC_CUSTOM_LOOP_UNROLL_HPP

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace llvm;

struct CustomLoopUnroll : public LoopPass {
    static char ID;
    CustomLoopUnroll() : LoopPass(ID) {}

    auto doInitialization(Loop *L, LPPassManager &LPM) -> bool override;
    auto runOnLoop(Loop *L, LPPassManager &LPM) -> bool override;
    void getAnalysisUsage(AnalysisUsage &AU) const override;
};

auto ReplaceSuccessor(BasicBlock *bb, BasicBlock *succ, BasicBlock *repl) -> bool;
auto UnrollNTimes(Loop *L, LPPassManager &LPM, unsigned UnrollCount, LoopInfoWrapperPass &LIWP)
    -> bool;

#endif
