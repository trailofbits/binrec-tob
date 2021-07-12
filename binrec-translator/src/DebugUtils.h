#ifndef BINREC_DEBUGUTILS_H
#define BINREC_DEBUGUTILS_H

#include "PassUtils.h"

#define DEBUG_FD STDERR_FILENO

using namespace llvm;

auto buildWrite(IRBuilder<> &b, Value *buf, Value *size) -> CallInst *;
auto buildWrite(IRBuilder<> &b, StringRef str) -> CallInst *;
auto buildWriteChar(IRBuilder<> &b, Value *c, unsigned count = 1) -> CallInst *;
auto buildWriteChar(IRBuilder<> &b, char c, unsigned count = 1) -> CallInst *;
auto buildWriteInt(IRBuilder<> &b, Value *i, unsigned base = 10) -> CallInst *;
auto buildWriteInt(IRBuilder<> &b, int32_t i, unsigned base = 10) -> CallInst *;
auto buildWriteUint(IRBuilder<> &b, Value *i, unsigned base = 10, unsigned zeropad = 0) -> CallInst *;
auto buildWriteUint(IRBuilder<> &b, uint32_t i, unsigned base = 10, unsigned zeropad = 0) -> CallInst *;
auto buildWriteHex(IRBuilder<> &b, Value *i, unsigned zeropad = 0) -> CallInst *;
auto buildWriteHex(IRBuilder<> &b, uint32_t i, unsigned zeropad = 0) -> CallInst *;
auto buildPad(IRBuilder<> &b, Value *nwritten, char c, unsigned width) -> CallInst *;

auto buildExit(IRBuilder<> &b, Value *status) -> CallInst *;
auto buildExit(IRBuilder<> &b, int32_t status) -> CallInst *;

#endif // BINREC_DEBUGUTILS_H
