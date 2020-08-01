#ifndef _DEBUGUTILS_H
#define _DEBUGUTILS_H

#include "PassUtils.h"

#define DEBUG_FD STDERR_FILENO

using namespace llvm;

CallInst *buildWrite(IRBuilder<> &b, Value *buf, Value *size);
CallInst *buildWrite(IRBuilder<> &b, StringRef str);
CallInst *buildWriteChar(IRBuilder<> &b, Value *c, unsigned count = 1);
CallInst *buildWriteChar(IRBuilder<> &b, char c, unsigned count = 1);
CallInst *buildWriteInt(IRBuilder<> &b, Value *i, unsigned base = 10);
CallInst *buildWriteInt(IRBuilder<> &b, int32_t i, unsigned base = 10);
CallInst *buildWriteUint(IRBuilder<> &b, Value *i, unsigned base = 10, unsigned zeropad = 0);
CallInst *buildWriteUint(IRBuilder<> &b, uint32_t i, unsigned base = 10, unsigned zeropad = 0);
CallInst *buildWriteHex(IRBuilder<> &b, Value *i, unsigned zeropad = 0);
CallInst *buildWriteHex(IRBuilder<> &b, uint32_t i, unsigned zeropad = 0);
CallInst *buildPad(IRBuilder<> &b, Value *nwritten, char c, unsigned width);

CallInst *buildExit(IRBuilder<> &b, Value *status);
CallInst *buildExit(IRBuilder<> &b, int32_t status);

#endif  // _DEBUGUTILS_H
