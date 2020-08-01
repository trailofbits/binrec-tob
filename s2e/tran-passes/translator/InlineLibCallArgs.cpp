#include <iostream>
#include <algorithm>

#include "InlineLibCallArgs.h"
#include "PassUtils.h"

using namespace llvm;

char InlineLibCallArgs::ID = 0;
static RegisterPass<InlineLibCallArgs> X("inline-lib-call-args",
        "S2E Inline arguments of library function calls with known signatures",
        false, false);

void InlineLibCallArgs::readLibcArgSizes() {
    std::ifstream f;
    fileOpen(f, runlibDir() + "/libc-argsizes");
    Signature sig;
    sig.stub = NULL;
    size_t argsize;

    while (f >> sig.name >> sig.isfloat >> sig.retsize) {
        sig.argsizes.clear();

        while (f.peek() != '\n') {
            failUnless(f >> argsize, "could not read argsize");
            sig.argsizes.push_back(argsize);
        }

        sigmap.insert(std::pair<std::string, Signature>(sig.name, sig));
    }
}

static inline IntegerType *makeType(size_t nbytes) {
    return IntegerType::get(getGlobalContext(), nbytes * 8);
}

static Function *getDummyUser(Module *m) {
    LLVMContext &ctx = m->getContext();
    FunctionType *fnTy = FunctionType::get(Type::getVoidTy(ctx), true);
    return cast<Function>(m->getOrInsertFunction("__dummy_user", fnTy));
}

static Type *getRetTy(Signature &sig, LLVMContext &ctx) {
    if (sig.isfloat) {
        return Type::getVoidTy(ctx);
        //switch (sig.retsize) {
        //    case 4: return Type::getFloatTy(ctx);
        //    case 8: return Type::getDoubleTy(ctx);
        //    default: ERROR("invalid float retsize"); exit(1);
        //}
    } else if (sig.retsize == 0) {
        return Type::getVoidTy(ctx);
    } else {
        return Type::getIntNTy(ctx, 8 * sig.retsize);
    }
}

Function *InlineLibCallArgs::getOrInsertStub(Signature &sig) {
    if (sig.stub == NULL) {
        LLVMContext &ctx = module->getContext();
        std::vector<Type*> argTypes;

        for (size_t argsize : sig.argsizes)
            argTypes.push_back(makeType(argsize));

        FunctionType *fnTy = FunctionType::get(getRetTy(sig, ctx), argTypes, false);
        sig.stub = Function::Create(fnTy, GlobalValue::ExternalLinkage,
                stubName(sig.name), module);
        sig.stub->addFnAttr(Attribute::Naked);
        sig.stub->addFnAttr(Attribute::NoInline);
    }

    return sig.stub;
}

static inline std::string sizeChar(size_t size) {
    switch (size) {
        case 1: return "b";
        case 2: return "w";
        case 4: return "l";
        case 8: return "q";
    }
    assert(!"unsupported argument size");
    return "X";
}

static inline Value *memFunc(Module *m, size_t size) {
    std::string fnname = "__ld" + sizeChar(size) + "_mmu";
    Type *retTy = Type::getIntNTy(m->getContext(), size * 8);
    Type *i32Ty = Type::getInt32Ty(m->getContext());
    Type *argTypes[] = {i32Ty, i32Ty};
    return m->getOrInsertFunction(fnname,
            FunctionType::get(retTy, argTypes, false));
}

static inline Value *peekArg(Module *m, IRBuilder<> &b, unsigned offset, size_t size) {
    Value *esp = b.CreateLoad(m->getNamedGlobal("R_ESP"), "esp");
    Value *addr = b.CreateAdd(esp, b.getInt32(offset), "addr");
    return b.CreateCall(memFunc(m, size), {addr, b.getInt32(1)}, "arg");
}

static void collectUses(Instruction *inst, std::list<Instruction*> &eraseList) {
    eraseList.push_front(inst);
    foreach_use_if(inst, Instruction, use, collectUses(use, eraseList));
}

void InlineLibCallArgs::replaceCall(Module &m, CallInst *call, Signature &sig) {
    DBG("replace helper_extern_stub call with __stub_" << sig.name);

    IRBuilder<> b(call);
    std::vector<Value*> args;
    unsigned offset = 0;

    for (size_t argsize : sig.argsizes) {
        if (argsize > 8) {
            DBG("  argument bigger than 8 bytes, abort");
            return;
        }

        args.push_back(peekArg(module, b, offset, argsize));
        offset += std::max((unsigned long)argsize, 4lu);
    }

    // Call trampoline function, returns to next instruction
    Value *ret = b.CreateCall(getOrInsertStub(sig), args);

    // Copy return value to virtual registers
    if (sig.retsize > 0) {
        if (sig.isfloat) {
            b.CreateCall(module->getFunction("virtualize_return_float"));
        }
        else if (sig.retsize <= 4) {
            b.CreateCall(module->getFunction("virtualize_return_i32"), ret);
        }
        else if (sig.retsize == 8) {
            b.CreateCall(module->getFunction("virtualize_return_i64"), ret);
        }
        else {
            ERROR("function " << sig.name << " has invalid return size " << sig.retsize);
            exit(1);
        }
    }

    // Remove old helper call
    std::list<Instruction*> eraseList;
    collectUses(call, eraseList);

    for (Instruction *inst : eraseList)
        inst->eraseFromParent();
}

bool InlineLibCallArgs::runOnModule(Module &m) {
    Function *helper = m.getFunction("helper_stub_trampoline");

    if (!helper)
        return false;

    module = &m;
    readLibcArgSizes();

    foreach_use_if(helper, CallInst, call, {
        if (MDNode *md = call->getMetadata("funcname")) {
            const std::string &funcname = cast<MDString>(md->getOperand(0))->getString().str();
            auto it = sigmap.find(funcname);
            //DBG();
            if (it != sigmap.end())
                replaceCall(m, call, it->second);
        }
    });

    // exit() should not return (this is used to remove destructor code)
    if (Function *exitStub = m.getFunction(stubName("exit"))) {
        DBG("found stub for exit(), marking it as noreturn");
        exitStub->addFnAttr(Attribute::NoReturn);
    }

    return true;
}
