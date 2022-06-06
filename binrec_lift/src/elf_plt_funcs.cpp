/*
 * for each stub address in the symbols file, create an internal function with
 * that name that jumps to the corresponding PLT address
 */

#include "error.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <cassert>
#include <fstream>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>

using namespace llvm;

namespace {
    auto implementPLTFun(Module &M, uint64_t Addr, StringRef Symbol) -> bool
    {
        Function *F = M.getFunction(Symbol);

        if (!F)
            return false;

        F->setLinkage(GlobalValue::InternalLinkage);
        F->addFnAttr(Attribute::Naked);
        F->addFnAttr(Attribute::NoInline);

        IRBuilder<> B(BasicBlock::Create(M.getContext(), "entry", F));
        Value *Target = B.CreateIntToPtr(B.getInt32(Addr), F->getType());
        std::vector<Value *> Args;
        std::transform(F->arg_begin(), F->arg_end(), std::back_inserter(Args), [](Argument &A) {
            return &A;
        });
        CallInst *RetVal = B.CreateCall(FunctionCallee{F->getFunctionType(), Target}, Args);
        RetVal->setTailCallKind(CallInst::TCK_MustTail);
        B.CreateRet(RetVal);

        return true;
    }
} // namespace

struct ELFPLTFunsPass : public ModulePass {
    static char ID;
    ELFPLTFunsPass() : ModulePass(ID) {}

    auto runOnModule(Module &M) -> bool override
    {
        std::ifstream F;
        uint64_t Addr = 0;
        std::string Symbol;
        bool Changed = false;

        s2eOpen(F, "symbols");

        while (F >> std::hex >> Addr >> Symbol)
            Changed |= implementPLTFun(M, Addr, Symbol);

        for (Function &F : M) {
            if (F.hasName() && F.empty() && !F.isIntrinsic() && !F.getName().startswith("__st") &&
                !F.getName().startswith("__ld"))
            {
                LLVM_ERROR(error) << "unresolved function: " << F.getName();
                throw binrec::lifting_error{"elf_plt_funcs", error};
            }
        }

        return Changed;
    }
};

char ELFPLTFunsPass::ID = 0;
namespace {
    RegisterPass<ELFPLTFunsPass>
        X("elf-plt-funs",
          "S2E Implement external function stubs with fallback to the PLT",
          false,
          false);
}
