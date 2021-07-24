#include "intrinsic_cleaner.hpp"
#include <llvm/IR/IRBuilder.h>
#include <vector>

using namespace binrec;
using namespace llvm;
using namespace std;

static void clean_uadd(Function &f)
{
    vector<User *> users{f.user_begin(), f.user_end()};
    for (User *u : users) {
        auto &call = *cast<CallInst>(u);
        IRBuilder<> irb{&call};
        Value *result = irb.CreateIntrinsic(
            Intrinsic::uadd_with_overflow,
            f.getArg(1)->getType(),
            {call.getOperand(1), call.getOperand(2)});
        irb.CreateStore(irb.CreateExtractValue(result, 0), call.getOperand(0));
        Value *bit = irb.CreateExtractValue(result, 1);
        call.replaceAllUsesWith(bit);
        call.eraseFromParent();
    }
    f.eraseFromParent();
}

/*static void clean_div(Function &f)
{
    // TODO: LLVM cannot encode an x86 division in that it cannot divide a 16 bit by an 8 bit value,
    // resulting in an 8 bit value causing an exception if the result is greater than 8 bit. We
    // ignore this for now, but later on we should probably add an intrinsic, or rewrite the IR such
    // that we don't have to truncate.
    vector<User *> users{f.user_begin(), f.user_end()};
    for (User *u : users) {
        auto &call = *cast<CallInst>(u);
        IRBuilder<> irb{&call};
        Value *numerator = irb.CreateLoad(f.getParent()->getNamedGlobal("R_EAX"));
        numerator = irb.CreateTrunc(numerator, irb.getInt16Ty());
        Value *denominator = call.getArgOperand(0);
        denominator =
            irb.CreateZExt(irb.CreateTrunc(denominator, irb.getInt8Ty()), irb.getInt16Ty());
        Value *quotient = irb.CreateZExt(
            irb.CreateTrunc(irb.CreateUDiv(numerator, denominator), irb.getInt8Ty()),
            irb.getInt16Ty());
        Value *remainder = irb.CreateZExt(
            irb.CreateTrunc(irb.CreateURem(numerator, denominator), irb.getInt8Ty()),
            irb.getInt16Ty());
        Value *result = call.replaceAllUsesWith(result);
        call.eraseFromParent();
    }
    f.eraseFromParent();
}*/

static void clean_ctlz(Function &f)
{
    vector<User *> users{f.user_begin(), f.user_end()};
    for (User *u : users) {
        auto &call = *cast<CallInst>(u);
        IRBuilder<> irb{&call};
        Value *result = irb.CreateIntrinsic(
            Intrinsic::ctlz,
            f.getArg(0)->getType(),
            {call.getOperand(0), irb.getInt1(false)});
        call.replaceAllUsesWith(irb.CreateTrunc(result, irb.getInt8Ty()));
        call.eraseFromParent();
    }
    f.eraseFromParent();
}

static void clean_memcpy(Function &f)
{
    vector<User *> users{f.user_begin(), f.user_end()};
    for (User *u : users) {
        auto &call = *cast<CallInst>(u);
        IRBuilder<> irb{&call};
        Value *dest = irb.CreateIntToPtr(call.getOperand(0), irb.getInt8PtrTy());
        irb.CreateIntrinsic(
            Intrinsic::memcpy,
            {irb.getInt8PtrTy(), irb.getInt8PtrTy(), irb.getInt32Ty()},
            {dest,
             irb.CreateIntToPtr(call.getOperand(1), irb.getInt8PtrTy()),
             call.getOperand(2),
             irb.getInt1(false)});
        call.replaceAllUsesWith(irb.CreatePtrToInt(dest, irb.getInt32Ty()));
        call.eraseFromParent();
    }
    f.eraseFromParent();
}

static void clean_memmove(Function &f)
{
    vector<User *> users{f.user_begin(), f.user_end()};
    for (User *u : users) {
        auto &call = *cast<CallInst>(u);
        IRBuilder<> irb{&call};
        Value *dest = irb.CreateIntToPtr(call.getOperand(0), irb.getInt8PtrTy());
        irb.CreateIntrinsic(
            Intrinsic::memmove,
            {irb.getInt8PtrTy(), irb.getInt8PtrTy(), irb.getInt32Ty()},
            {dest,
             irb.CreateIntToPtr(call.getOperand(1), irb.getInt8PtrTy()),
             call.getOperand(2),
             irb.getInt1(false)});
        call.replaceAllUsesWith(irb.CreatePtrToInt(dest, irb.getInt32Ty()));
        call.eraseFromParent();
    }
    f.eraseFromParent();
}

static void clean_memset(Function &f)
{
    vector<User *> users{f.user_begin(), f.user_end()};
    for (User *u : users) {
        auto &call = *cast<CallInst>(u);
        IRBuilder<> irb{&call};
        Value *dest = irb.CreateIntToPtr(call.getOperand(0), irb.getInt8PtrTy());
        irb.CreateIntrinsic(
            Intrinsic::memset,
            {irb.getInt8PtrTy(), irb.getInt32Ty()},
            {dest,
             irb.CreateTrunc(call.getOperand(1), irb.getInt8Ty()),
             call.getOperand(2),
             irb.getInt1(false)});
        call.replaceAllUsesWith(irb.CreatePtrToInt(dest, irb.getInt32Ty()));
        call.eraseFromParent();
    }
    f.eraseFromParent();
}

auto IntrinsicCleanerPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    vector<reference_wrapper<Function>> functions{m.begin(), m.end()};
    for (Function &f : functions) {
        StringRef name = f.getName();
        if (name.startswith("uadd.")) {
            clean_uadd(f);
        } else if (name.startswith("countLeadingZeros")) {
            clean_ctlz(f);
        } else if (name == "memcpy") {
            clean_memcpy(f);
        } else if (name == "memmove") {
            clean_memmove(f);
        } else if (name == "memset") {
            clean_memset(f);
        }
    }

    return PreservedAnalyses::none();
}
