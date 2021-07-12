#ifndef BINREC_SEGFAULTDEBUG_H
#define BINREC_SEGFAULTDEBUG_H

#include <llvm/IR/PassManager.h>

using namespace llvm;

struct SegfaultDebug : public FunctionPass {
    static char ID;
    SegfaultDebug() : FunctionPass(ID) {}

    auto runOnFunction(Function &g) -> bool override;
};

#endif
