#include "extern_plt.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/Function.h>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace binrec;
using namespace llvm;

static auto find_trampoline_call(BasicBlock &bb) -> CallInst *
{
    for (Instruction &inst : bb) {
        if (auto *call = dyn_cast<CallInst>(&inst)) {
            Function *f = call->getCalledFunction();
            if (f && f->hasName() && f->getName() == "helper_stub_trampoline")
                return call;
        }
    }
    assert(!"no call to helper_stub_trampoline");
}

// NOLINTNEXTLINE
auto ExternPLTPass::run(Function &f, FunctionAnalysisManager &am) -> PreservedAnalyses
{
    auto &m = *f.getParent();
    auto *nonlib_setjmp = m.getFunction("nonlib_setjmp");
    assert(nonlib_setjmp);
    auto *nonlib_longjmp = m.getFunction("nonlib_longjmp");
    assert(nonlib_longjmp);
    auto *helper = m.getFunction("helper_extern_stub");
    assert(helper);

    bool changed = false;

    for (BasicBlock &bb : f) {
        MDNode *md = getBlockMeta(&bb, "extern_symbol");
        if (!md) {
            continue;
        }

        // Strip optional "library_name::" prefix from symbol
        // Value * V = MetadataAsValue::get(md->getContext(),
        // md->getOperand(0).get());
        StringRef sym;
        if (const MDString *mds = dyn_cast<MDString>(md->getOperand(0).get())) {
            sym = mds->getString();
        }

        StringRef funcname = sym.substr(sym.rfind(':') + 1);
        DBG("generating fallback to PLT for " << funcname << " at "
                                              << utohexstr(getBlockAddress(&bb)));

        // Remove all instructions except first and return
        std::list<Instruction *> erase_list;

        for (Instruction &inst : bb) {
            if (&inst != &bb.front() && !isa<ReturnInst>(&inst)) {
                erase_list.push_front(&inst);
            }
        }

        for (Instruction *inst : erase_list) {
            inst->eraseFromParent();
        }

        // Insert call to helper_extern_stub
        IRBuilder<> b(bb.getTerminator());

        CallInst *call;
        StringRef jump_type;
        if (funcname.endswith("setjmp")) {
            DBG("_setjmp is replaced with nonlib version");
            call = b.CreateCall(nonlib_setjmp);
            jump_type = "setjmp_type";
        } else if (funcname.endswith("longjmp") || funcname.endswith("longjmp_chk")) {
            DBG("longjmp is replaced with nonlib version");
            call = b.CreateCall(nonlib_longjmp);
            jump_type = "longjmp_type";
        } else {
            call = b.CreateCall(helper);

            if (&bb == &f.getEntryBlock()) {
                // Note(FPar): When recovering functions, the inline stubs pass does not
                // inline the blocks. We need to trim off the successors (which is the code
                // that would load the values into the global offset table).

                // Q(FPar): I conservatively only added this to helper_extern_stub. Do we
                // need this with setjmp and longjmp, too? Probably not, because those do
                // not run plt code.
                setBlockSuccs(&bb, {});
            }
        }

        // Inline the function call so that we can put metadata on the
        // helper_stub_trampoline call inside its body
        InlineFunctionInfo info;
        if (!InlineFunction(*call, info).isSuccess()) {
            errs() << "could not inline call:" << *call << "\n";
            exit(1);
        }

        // Attach symbol as metadata to trampoline call for optional use by
        // -inline-libcall-args
        LLVMContext &ctx = bb.getContext();
        if (!jump_type.empty()) {
            DBG("inlined_lib_func: " << bb.getName());
            setBlockMeta(&bb, "inlined_lib_func", MDNode::get(ctx, MDString::get(ctx, jump_type)));
            // setBlockMeta(&bb, "inlined_lib_func", MDNode::get(ctx,
            // MDString::get(ctx, jumpType)));
        } else {
            find_trampoline_call(bb)->setMetadata(
                "funcname",
                MDNode::get(ctx, MDString::get(ctx, funcname)));
        }
        changed = true;
    }
    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
