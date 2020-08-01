/*
 * Assume the presence of a function "helper_extern_call" that takes a function
 * pointer argument and:
 * - reads and saves a return PC from *@R_ESP
 * - backs up concrete registers
 * - copies virtual registers to concrete registers
 * - calls the external function in the first argument
 * - copies concrete values to virtual registers
 * - restores original concrete registers
 * - returns the saved return PC
 */

#include "ExternPLT.h"
#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char ExternPLTPass::ID = 0;
static RegisterPass<ExternPLTPass> X("extern-plt",
        "S2E Patch extern function calls to use the existing PLT", false, false);

static GlobalVariable *getRegister(Module &m, StringRef name) {
    GlobalVariable *reg = m.getNamedGlobal(name);
    if (!reg) {
        Type *ty = Type::getInt32Ty(m.getContext());
        reg = new GlobalVariable(m, ty, false, GlobalValue::InternalLinkage,
                ConstantInt::get(ty, 0), name);
    }
    return reg;
}

bool ExternPLTPass::doInitialization(Module &m) {
    nonlib_setjmp = m.getFunction("nonlib_setjmp");
    assert(nonlib_setjmp);
    nonlib_longjmp = m.getFunction("nonlib_longjmp");
    assert(nonlib_longjmp);
    helper = m.getFunction("helper_extern_stub");
    assert(helper);
    return true;
}

static CallInst *findTrampolineCall(BasicBlock &bb) {
    for (Instruction &inst : bb) {
        ifcast(CallInst, call, &inst) {
            Function *f = call->getCalledFunction();
            if (f && f->hasName() && f->getName() == "helper_stub_trampoline")
                return call;
        }
    }
    assert(!"no call to helper_stub_trampoline");
}

bool ExternPLTPass::runOnBasicBlock(BasicBlock &bb) {
    MDNode *md = getBlockMeta(&bb, "extern_symbol");
    if (!md)
        return false;

    // Strip optional "library_name::" prefix from symbol
    //Value * V = MetadataAsValue::get(md->getContext(), md->getOperand(0).get());
    StringRef sym;
    if (const MDString *MDS = dyn_cast<MDString>(md->getOperand(0).get())) {
        //llvm::errs() << "AA: " << MDS->getString() << "\n";
        sym = MDS->getString();
    }

    StringRef funcname = sym.substr(sym.rfind(':') + 1);
    DBG("generating fallback to PLT for " << funcname <<
            " at " << intToHex(getBlockAddress(&bb)));

    // Remove all instructions except first and return
    std::list<Instruction*> eraseList;

    for (Instruction &inst : bb) {
        if (&inst != bb.begin() && !isa<ReturnInst>(&inst))
            eraseList.push_front(&inst);
    }

    for (Instruction *inst : eraseList)
        inst->eraseFromParent();

    // Insert call to helper_extern_stub
    Module *m = bb.getParent()->getParent();
    IRBuilder<> b(bb.getTerminator());

    CallInst *call = nullptr;
//    bool setOrLongJump = false;
    StringRef jumpType;
    if(funcname.endswith("setjmp")){
        DBG("_setjmp is replaced with nonlib version");
        call = b.CreateCall(nonlib_setjmp);
//        setOrLongJump = true;
        jumpType = "setjmp_type";
    }
    else if(funcname.endswith("longjmp") || funcname.endswith("longjmp_chk")){
        DBG("longjmp is replaced with nonlib version");
        call = b.CreateCall(nonlib_longjmp);
//        setOrLongJump = true;
        jumpType = "longjmp_type";
    }
    else{
        call = b.CreateCall(helper);
    }

    // Inline the function call so that we can put metadata on the
    // helper_stub_trampoline call inside its body
    InlineFunctionInfo info;
    if (!InlineFunction(call, info)) {
        errs() << "could not inline call:" << *call << "\n";
        exit(1);
    }

    // Attach symbol as metadata to trampoline call for optional use by
    // -inline-libcall-args
    LLVMContext &ctx = bb.getContext();
    if(!jumpType.empty()){
        DBG("inlined_lib_func: " << bb.getName());
        setBlockMeta(&bb, "inlined_lib_func", MDNode::get(ctx, MDString::get(ctx, jumpType)));
        //setBlockMeta(&bb, "inlined_lib_func", MDNode::get(ctx, MDString::get(ctx, jumpType)));
    }else{
        findTrampolineCall(bb)->setMetadata("funcname",
            MDNode::get(ctx, MDString::get(ctx, funcname)));
    }
    return true;
}
