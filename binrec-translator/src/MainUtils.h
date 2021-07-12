#ifndef BINREC_MAINUTILS_H
#define BINREC_MAINUTILS_H

#include <llvm/IR/BasicBlock.h>

#define STACK_SIZE (1024 * 1024 * 16) // 16MB

using namespace llvm;

void callWrapper(BasicBlock *bb);
auto allocateStack(BasicBlock *bb) -> Value *;
void freeStack(Value *stack, BasicBlock *bb);

#endif // BINREC_MAINUTILS_H
