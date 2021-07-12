#include "DebugUtils.h"
#include "PassUtils.h"
#include <unistd.h>

using namespace llvm;

static inline auto getModule(IRBuilder<> &b) -> Module * { return b.GetInsertBlock()->getParent()->getParent(); }

static auto getFunc(IRBuilder<> &b, StringRef name) -> Function * {
    if (Constant *f = getModule(b)->getFunction(name))
        return cast<Function>(f);

    errs() << "debug function \"" << name << "\" not found\n";
    exit(1);
}

auto buildWrite(IRBuilder<> &b, Value *buf, Value *size) -> CallInst * {
    Value *args[] = {b.getInt32(DEBUG_FD), buf, size};
    return b.CreateCall(getFunc(b, "mywrite"), args);
}

auto buildWrite(IRBuilder<> &b, StringRef str) -> CallInst * {
    return buildWrite(b, b.CreateGlobalStringPtr(str), b.getInt32(str.size()));
}

auto buildWriteChar(IRBuilder<> &b, Value *c, unsigned count) -> CallInst * {
    Value *args[] = {b.getInt32(DEBUG_FD), c, b.getInt32(count)};
    return b.CreateCall(getFunc(b, "mywrite_chars"), args);
}

auto buildWriteChar(IRBuilder<> &b, char c, unsigned count) -> CallInst * {
    return buildWriteChar(b, b.getInt8(c), count);
}

auto buildWriteInt(IRBuilder<> &b, Value *i, unsigned base) -> CallInst * {
    Value *args[] = {b.getInt32(DEBUG_FD), i, b.getInt32(base)};
    return b.CreateCall(getFunc(b, "mywrite_int"), args);
}

auto buildWriteInt(IRBuilder<> &b, int32_t i, unsigned base) -> CallInst * {
    return buildWriteInt(b, ConstantInt::get(b.getInt32Ty(), i, true), base);
}

auto buildWriteUint(IRBuilder<> &b, Value *i, unsigned base, unsigned zeropad) -> CallInst * {
    std::vector<Value *> args = {b.getInt32(DEBUG_FD), i, b.getInt32(base)};
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

auto buildWriteUint(IRBuilder<> &b, uint32_t i, unsigned base, unsigned zeropad) -> CallInst * {
    return buildWriteUint(b, b.getInt32(i), base, zeropad);
}

auto buildWriteHex(IRBuilder<> &b, Value *i, unsigned zeropad) -> CallInst * {
    return buildWriteUint(b, i, 16, zeropad);
}

auto buildWriteHex(IRBuilder<> &b, uint32_t i, unsigned zeropad) -> CallInst * {
    return buildWriteUint(b, b.getInt32(i), 16, zeropad);
}

auto buildPad(IRBuilder<> &b, Value *nwritten, char c, unsigned width) -> CallInst * {
    Value *args[] = {b.getInt32(DEBUG_FD), nwritten, b.getInt8(c), b.getInt32(width)};
    return b.CreateCall(getFunc(b, "pad_right"), args);
}

auto buildExit(IRBuilder<> &b, Value *status) -> CallInst * { return b.CreateCall(getFunc(b, "myexit"), status); }

auto buildExit(IRBuilder<> &b, int32_t status) -> CallInst * {
    return buildExit(b, ConstantInt::get(b.getInt32Ty(), status, true));
}
