#include <unistd.h>

#include "DebugUtils.h"
#include "PassUtils.h"

using namespace llvm;

static inline Module *getModule(IRBuilder<> &b) {
    return b.GetInsertBlock()->getParent()->getParent();
}

static Function *getFunc(IRBuilder<> &b, StringRef name) {
    if (Constant *f = getModule(b)->getFunction(name))
        return cast<Function>(f);

    errs() << "debug function \"" << name << "\" not found\n";
    exit(1);
}

CallInst *buildWrite(IRBuilder<> &b, Value *buf, Value *size) {
    Value *args[] = {b.getInt32(DEBUG_FD), buf, size};
    return b.CreateCall(getFunc(b, "mywrite"), args);
}

CallInst *buildWrite(IRBuilder<> &b, StringRef str) {
    return buildWrite(b, b.CreateGlobalStringPtr(str), b.getInt32(str.size()));
}

CallInst *buildWriteChar(IRBuilder<> &b, Value *c, unsigned count) {
    Value *args[] = {b.getInt32(DEBUG_FD), c, b.getInt32(count)};
    return b.CreateCall(getFunc(b, "mywrite_chars"), args);
}

CallInst *buildWriteChar(IRBuilder<> &b, char c, unsigned count) {
    return buildWriteChar(b, b.getInt8(c), count);
}

CallInst *buildWriteInt(IRBuilder<> &b, Value *i, unsigned base) {
    Value *args[] = {b.getInt32(DEBUG_FD), i, b.getInt32(base)};
    return b.CreateCall(getFunc(b, "mywrite_int"), args);
}

CallInst *buildWriteInt(IRBuilder<> &b, int32_t i, unsigned base) {
    return buildWriteInt(b, ConstantInt::get(b.getInt32Ty(), i, true), base);
}

CallInst *buildWriteUint(IRBuilder<> &b, Value *i, unsigned base, unsigned zeropad) {
    std::vector<Value*> args = {b.getInt32(DEBUG_FD), i, b.getInt32(base)};
    StringRef fn;

    if (zeropad) {
        fn = "mywrite_uint_pad";
        args.push_back(b.getInt8('0'));
        args.push_back(b.getInt32(zeropad));
    } else {
        fn = "mywrite_uint";
    }

    return b.CreateCall(getFunc(b, fn), args);
}

CallInst *buildWriteUint(IRBuilder<> &b, uint32_t i, unsigned base, unsigned zeropad) {
    return buildWriteUint(b, b.getInt32(i), base, zeropad);
}

CallInst *buildWriteHex(IRBuilder<> &b, Value *i, unsigned zeropad) {
    return buildWriteUint(b, i, 16, zeropad);
}

CallInst *buildWriteHex(IRBuilder<> &b, uint32_t i, unsigned zeropad) {
    return buildWriteUint(b, b.getInt32(i), 16, zeropad);
}

CallInst *buildPad(IRBuilder<> &b, Value *nwritten, char c, unsigned width) {
    Value *args[] = {b.getInt32(DEBUG_FD), nwritten, b.getInt8(c), b.getInt32(width)};
    return b.CreateCall(getFunc(b, "pad_right"), args);
}

CallInst *buildExit(IRBuilder<> &b, Value *status) {
    return b.CreateCall(getFunc(b, "myexit"), status);
}

CallInst *buildExit(IRBuilder<> &b, int32_t status) {
    return buildExit(b, ConstantInt::get(b.getInt32Ty(), status, true));
}
