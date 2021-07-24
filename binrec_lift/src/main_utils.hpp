#ifndef BINREC_MAIN_UTILS_HPP
#define BINREC_MAIN_UTILS_HPP

#include <llvm/IR/BasicBlock.h>

#define STACK_SIZE (1024 * 1024 * 16) // 16MB

using namespace llvm;

void callWrapper(BasicBlock *bb);
auto allocateStack(BasicBlock *bb) -> Value *;
void freeStack(Value *stack, BasicBlock *bb);

#endif
