#ifndef _UNTANGLE_INTERPRETER_H
#define _UNTANGLE_INTERPRETER_H

#include <llvm/Analysis/LoopPass.h>
#include <llvm/Support/CommandLine.h>

using namespace llvm;

extern cl::opt<std::string> InterpreterEntry;

extern cl::opt<std::string> VpcGlobal;

extern cl::list<std::string> VpcTraceFiles;

typedef std::pair<BasicBlock*, BasicBlock*> HeaderLatchPair;

struct UntangleInterpreter : public LoopPass {
    static char ID;
    UntangleInterpreter() : LoopPass(ID), errBlock(NULL) {}

    virtual bool runOnLoop(Loop *L, LPPassManager &LPM);
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

private:
    GlobalVariable *vpc;
    Loop *interpreter;
    std::map<unsigned, HeaderLatchPair> vpcMap;
    BasicBlock *errBlock;

    const HeaderLatchPair &getOrCreatePair(const unsigned vpcValue);
};

#endif  // _UNTANGLE_INTERPRETER_H
