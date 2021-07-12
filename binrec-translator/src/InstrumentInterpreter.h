#ifndef BINREC_INSTRUMENTINTERPRETER_H
#define BINREC_INSTRUMENTINTERPRETER_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct InstrumentInterpreter : public ModulePass {
    static char ID;
    InstrumentInterpreter() : ModulePass(ID) {}

    auto doInitialization(Module &m) -> bool override;
    auto runOnModule(Module &m) -> bool override;
};

#endif // BINREC_INSTRUMENTINTERPRETER_H
