#ifndef _INSTRUMENT_INTERPRETER_H
#define _INSTRUMENT_INTERPRETER_H

#include <llvm/Pass.h>

using namespace llvm;

struct InstrumentInterpreter : public ModulePass {
    static char ID;
    InstrumentInterpreter() : ModulePass(ID) {}

    virtual bool doInitialization(Module &m);
    virtual bool runOnModule(Module &m);
};

#endif  // _INSTRUMENT_INTERPRETER_H
