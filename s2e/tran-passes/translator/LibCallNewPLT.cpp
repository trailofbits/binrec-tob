#include <iostream>
#include <algorithm>
//#include "llvm/IR/TypeBuilder.h"
#include "LibCallNewPLT.h"
#include "PassUtils.h"
#include "llvm/IR/Constants.h"

using namespace llvm;

char LibCallNewPLT::ID = 0;
static RegisterPass<LibCallNewPLT> X("lib-call-new-plt",
        "replace fn ptr arg of helper_stub_trampoline with newplt function ptr",
        false, false);


bool LibCallNewPLT::runOnModule(Module &m) {
    LLVMContext &ctx = m.getContext();
    Function *helper = m.getFunction("helper_stub_trampoline");
    if (!helper){
        DBG("found no calls to helper_stub_trampoline");
        return false;
    }
    //foreach(use, helper->uses()){true
        //DBG(*use->getUser());
    //}
    auto argType = helper->getFunctionType()->getFunctionParamType(3);
    foreach_use_if(helper, CallInst, call, {
        if (MDNode *md = call->getMetadata("funcname")) {
            const std::string &funcname = cast<MDString>(md->getOperand(0))->getString().str();
            //following dbg is kinda spammy
            //DBG("Replacing fn pointer arg with ptr to plt in helper call to: " << funcname);

            //I don't think we we need the real signature of the function, just use i32 ()
            //if this isn't true, do something like the following
            //std::vector<Type*> argTypes;
            //for (size_t argsize : sig.argsizes)
            //argTypes.push_back(makeType(argsize));
            //FunctionType *fnTy = FunctionType::get(getRetTy(sig, ctx), argTypes, false);
            Constant* newFc;
            if (!(newFc = m.getFunction(funcname))){
                DBG("Adding function declaration for new name: " << funcname);
                //not sure if marking varidaic (the true at the end) does anything
                newFc = m.getOrInsertFunction(funcname, FunctionType::get(Type::getInt32Ty(ctx), false));
            }
            Function* newF = dyn_cast<Function>(newFc);
            newF->addFnAttr(Attribute::Naked);
            newF->addFnAttr(Attribute::NoInline);
            //fPtr->print(llvm::errs());
            //replace function pointer arg of helper_stub_trampoline wiht that ptr
            //Constant* fPtr = ConstantExpr::getBitCast(newF,argType );
            //call->setArgOperand(3, fPtr);
            Value* tPtr = CastInst::CreatePointerCast(newF, IntegerType::get(ctx, 32), "",call);
            call->setArgOperand(3, tPtr);
        }
    });
    return true;
}
