#include "PassUtils.h"
#include "RegisterPasses.h"
#include <algorithm>
#include <llvm/IR/Constants.h>
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
/// replace fn ptr arg of helper_stub_trampoline with newplt function ptr
class LibCallNewPLTPass : public PassInfoMixin<LibCallNewPLTPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
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
                    // DBG("Replacing fn pointer arg with ptr to plt in helper call to: " << funcname);

                    // I don't think we we need the real signature of the function, just use i32 ()
                    // if this isn't true, do something like the following
                    // std::vector<Type*> argTypes;
                    // for (size_t argsize : sig.argsizes)
                    // argTypes.push_back(makeType(argsize));
                    // FunctionType *fnTy = FunctionType::get(getRetTy(sig, ctx), argTypes, false);
                    Constant *newFc = nullptr;
                    if (!(newFc = m.getFunction(funcname))) {
                        DBG("Adding function declaration for new name: " << funcname);
                        // not sure if marking varidaic (the true at the end) does anything
                        newFc = cast<Constant>(
                            m.getOrInsertFunction(funcname, FunctionType::get(Type::getInt32Ty(ctx), false))
                                .getCallee());
                    }
                    auto *newF = dyn_cast<Function>(newFc);
                    newF->addFnAttr(Attribute::Naked);
                    newF->addFnAttr(Attribute::NoInline);
                    // fPtr->print(llvm::errs());
                    // replace function pointer arg of helper_stub_trampoline wiht that ptr
                    // Constant* fPtr = ConstantExpr::getBitCast(newF,argType );
                    // call->setArgOperand(3, fPtr);
                    Value *tPtr = CastInst::CreatePointerCast(newF, IntegerType::get(ctx, 32), "", call);
                    call->setArgOperand(3, tPtr);
                }
            }
        }
        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addLibCallNewPLTPass(ModulePassManager &mpm) { mpm.addPass(LibCallNewPLTPass()); }
