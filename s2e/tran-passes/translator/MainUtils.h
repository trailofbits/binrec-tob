#ifndef _MAINUTILS_H
#define _MAINUTILS_H

#include "llvm/IR/BasicBlock.h"

#define STACK_SIZE (1024 * 1024 * 16)  // 16MB

using namespace llvm;

void callWrapper(BasicBlock *bb);
Value *allocateStack(BasicBlock *bb);
void freeStack(Value *stack, BasicBlock *bb);

#endif  // _MAINUTILS_H
