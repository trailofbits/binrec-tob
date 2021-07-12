#include "InlineLibCallArgs.h"
#include "PassUtils.h"
#include <fstream>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
struct Signature {
    std::string name;
    bool isfloat{};
    size_t retsize{};
    std::vector<size_t> argsizes;
    Function *stub{};
};

auto makeType(LLVMContext &ctx, size_t nbytes) -> IntegerType * { return IntegerType::get(ctx, nbytes * 8); }

auto getRetTy(Signature &sig, LLVMContext &ctx) -> Type * {
    if (sig.isfloat) {
        return Type::getVoidTy(ctx);
        // switch (sig.retsize) {
        //    case 4: return Type::getFloatTy(ctx);
        //    case 8: return Type::getDoubleTy(ctx);
        //    default: ERROR("invalid float retsize"); exit(1);
        //}
    }
    if (sig.retsize == 0) {
        return Type::getVoidTy(ctx);
    }
    return Type::getIntNTy(ctx, 8 * sig.retsize);
}

auto stubName(const std::string &base) -> std::string { return "__stub_" + base; }

auto peekArg(Module *m, IRBuilder<> &irb, unsigned offset, size_t size) -> Value * {
    Value *esp = irb.CreateLoad(m->getNamedGlobal("R_ESP"), "esp");
    Type *retTy = Type::getIntNTy(m->getContext(), size * 8);
    Value *ptr = irb.CreateIntToPtr(irb.CreateAdd(esp, irb.getInt32(offset)), retTy->getPointerTo());
    return irb.CreateLoad(ptr, "arg");
}

void collectUses(Instruction *inst, std::list<Instruction *> &eraseList) {
    eraseList.push_front(inst);
    for (User *use : inst->users()) {
        if (auto *instUse = dyn_cast<Instruction>(use))
            collectUses(instUse, eraseList);
    }
}

auto readLibcArgSizes() -> std::map<std::string, Signature> {
    std::map<std::string, Signature> sigmap;

    std::ifstream f;
    fileOpen(f, runlibDir() + "/libc-argsizes");

    Signature sig;
    while (f >> sig.name >> sig.isfloat >> sig.retsize) {
        sig.argsizes.clear();

        while (f.peek() != '\n') {
            size_t argsize = 0;
            failUnless(static_cast<bool>(f >> argsize), "could not read argsize");
            sig.argsizes.push_back(argsize);
        }

        sigmap.insert(std::pair<std::string, Signature>(sig.name, sig));
    }

    return sigmap;
}

auto getOrInsertStub(Module &m, Signature &sig) -> Function * {
    if (sig.stub == nullptr) {
        LLVMContext &ctx = m.getContext();
        std::vector<Type *> argTypes;

        for (size_t argsize : sig.argsizes)
            argTypes.push_back(makeType(ctx, argsize));

        FunctionType *fnTy = FunctionType::get(getRetTy(sig, ctx), argTypes, false);
        sig.stub = Function::Create(fnTy, GlobalValue::ExternalLinkage, stubName(sig.name), &m);
        sig.stub->addFnAttr(Attribute::NoInline);
    }

    return sig.stub;
}

void replaceCall(Module &m, CallInst *call, Signature &sig) {
    DBG("replace helper_extern_stub call with __stub_" << sig.name);

    IRBuilder<> b(call);
    std::vector<Value *> args;
    unsigned offset = 0;

    for (size_t argsize : sig.argsizes) {
        if (argsize > 8) {
            DBG("  argument bigger than 8 bytes, abort");
            return;
        }

        args.push_back(peekArg(&m, b, offset, argsize));
        offset += std::max((unsigned long)argsize, 4lu);
    }

    // Call trampoline function, returns to next instruction
    Value *ret = b.CreateCall(getOrInsertStub(m, sig), args);

    // Copy return value to virtual registers
    if (sig.retsize > 0) {
        if (sig.isfloat) {
            b.CreateCall(m.getFunction("virtualize_return_float"));
        } else if (sig.retsize <= 4) {
            b.CreateCall(m.getFunction("virtualize_return_i32"), ret);
        } else if (sig.retsize == 8) {
            b.CreateCall(m.getFunction("virtualize_return_i64"), ret);
        } else {
            ERROR("function " << sig.name << " has invalid return size " << sig.retsize);
            exit(1);
        }
    }

    // Remove old helper call
    std::list<Instruction *> eraseList;
    collectUses(call, eraseList);

    for (Instruction *inst : eraseList)
        inst->eraseFromParent();
}
} // namespace

// NOLINTNEXTLINE
auto InlineLibCallArgsPass::run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
    Function *helper = m.getFunction("helper_stub_trampoline");

    if (!helper)
        return PreservedAnalyses::all();

    auto sigmap = readLibcArgSizes();

    for (User *use : helper->users()) {
        if (auto *call = dyn_cast<CallInst>(use)) {
            if (MDNode *md = call->getMetadata("funcname")) {
                const std::string &funcname = cast<MDString>(md->getOperand(0))->getString().str();
                auto it = sigmap.find(funcname);
                if (it != sigmap.end())
                    replaceCall(m, call, it->second);
            }
        }
    }

    // exit() should not return (this is used to remove destructor code)
    if (Function *exitStub = m.getFunction(stubName("exit"))) {
        DBG("found stub for exit(), marking it as noreturn");
        exitStub->addFnAttr(Attribute::NoReturn);
    }

    return PreservedAnalyses::none();
}