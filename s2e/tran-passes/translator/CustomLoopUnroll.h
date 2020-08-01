#ifndef _CUSTOM_LOOP_UNROLL_H
#define _CUSTOM_LOOP_UNROLL_H

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace llvm;

struct CustomLoopUnroll : public LoopPass {
    static char ID;
    CustomLoopUnroll() : LoopPass(ID) {}

    virtual bool doInitialization(Loop *L, LPPassManager &LPM);
    virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;
};

bool ReplaceSuccessor(BasicBlock *bb, BasicBlock *succ, BasicBlock *repl);
bool UnrollNTimes(Loop *L, LPPassManager &LPM, unsigned UnrollCount, LoopInfoWrapperPass &LIWP);

#endif  // _CUSTOM_LOOP_UNROLL_H
