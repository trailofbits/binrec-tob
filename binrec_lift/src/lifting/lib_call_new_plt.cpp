#include "lib_call_new_plt.hpp"
#include "pass_utils.hpp"
#include <algorithm>
#include <llvm/IR/Constants.h>

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto LibCallNewPLTPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    LLVMContext &ctx = m.getContext();
    Function *helper = m.getFunction("helper_stub_trampoline");
    if (!helper) {
        DBG("found no calls to helper_stub_trampoline");
        return PreservedAnalyses::all();
    }
    for (User *use : helper->users()) {
        if (auto *call = dyn_cast<CallInst>(use)) {
            if (MDNode *md = call->getMetadata("funcname")) {
                const std::string &funcname = cast<MDString>(md->getOperand(0))->getString().str();
                // following dbg is kinda spammy
                // DBG("Replacing fn pointer arg with ptr to plt in helper call to: " <<
                // funcname);

                // I don't think we we need the real signature of the function, just use i32
                // () if this isn't true, do something like the following std::vector<Type*>
                // argTypes; for (size_t argsize : sig.argsizes)
                // argTypes.push_back(makeType(argsize));
                // FunctionType *fnTy = FunctionType::get(getRetTy(sig, ctx), argTypes,
                // false);
                Constant *new_fc = m.getFunction(funcname);
                if (!new_fc) {
                    DBG("Adding function declaration for new name: " << funcname);
                    // not sure if marking varidaic (the true at the end) does anything
                    new_fc = cast<Constant>(m.getOrInsertFunction(
                                                 funcname,
                                                 FunctionType::get(Type::getInt32Ty(ctx), false))
                                                .getCallee());
                }
                auto *new_f = dyn_cast<Function>(new_fc);
                new_f->addFnAttr(Attribute::Naked);
                new_f->addFnAttr(Attribute::NoInline);
                // fPtr->print(llvm::errs());
                // replace function pointer arg of helper_stub_trampoline wiht that ptr
                // Constant* fPtr = ConstantExpr::getBitCast(newF,argType );
                // call->setArgOperand(3, fPtr);
                Value *t_ptr =
                    CastInst::CreatePointerCast(new_f, IntegerType::get(ctx, 32), "", call);
                call->setArgOperand(3, t_ptr);
            }
        }
    }
    return PreservedAnalyses::none();
}
