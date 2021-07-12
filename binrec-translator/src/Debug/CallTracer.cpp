#include "CallTracer.h"
#include "IR/Selectors.h"
#include <llvm/IR/IRBuilder.h>
#include <unistd.h>

using namespace binrec;
using namespace llvm;

namespace {
auto getRtPrintf(Module &m) -> Function & {
    return *cast<Function>(
        m.getOrInsertFunction("binrec_rt_fdprintf",
                              FunctionType::get(Type::getInt32Ty(m.getContext()),
                                                {Type::getInt32Ty(m.getContext()), Type::getInt8PtrTy(m.getContext())},
                                                true))
            .getCallee());
}

auto makeDebugStr(Function &f) -> GlobalVariable * {
    SmallString<256> debugStr;
    debugStr += f.getName();
    debugStr += ": ";
    for (Argument &arg : f.args()) {
        debugStr += arg.getName();
        debugStr += "=%x,";
    }
    debugStr += "\n";
    Constant *str = ConstantDataArray::getString(f.getContext(), debugStr);
    auto *gv = new GlobalVariable{str->getType(), true, GlobalValue::PrivateLinkage, str};
    f.getParent()->getGlobalList().push_back(gv);
    return gv;
}
} // namespace

// NOLINTNEXTLINE
auto CallTracerPass::run(Module &m, ModuleAnalysisManager &) -> llvm::PreservedAnalyses {
    Function &rtPrintf = getRtPrintf(m);

    for (Function &f : LiftedFunctions{m}) {
        IRBuilder<> irb{&f.getEntryBlock().front()};
        GlobalVariable *debugStr = makeDebugStr(f);
        SmallVector<Value *, 16> args;
        args.push_back(irb.getInt32(STDERR_FILENO));
        args.push_back(irb.CreateInBoundsGEP(debugStr, {irb.getInt32(0), irb.getInt32(0)}));
        for (Argument &arg : f.args()) {
            Value *argVal = arg.getType()->isPointerTy() ? cast<Value>(irb.CreateLoad(&arg)) : &arg;
            args.push_back(argVal);
        }
        irb.CreateCall(&rtPrintf, args);
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
