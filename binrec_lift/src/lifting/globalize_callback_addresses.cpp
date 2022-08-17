#include "globalize_callback_addresses.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "ir/selectors.hpp"
#include <algorithm>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <vector>

using namespace llvm;

namespace binrec {

auto GlobalizeCallbackAddresses::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses {
    bool changed = false;
    IntegerType *i32Ty = IntegerType::getInt32Ty(m.getContext());
    IRBuilder<> b(m.getContext());

    DBG("[globalize_callback_addresses] running");

    for (Function &func : m) {
        if (!is_lifted_function(func)) {
            continue;
        }

        unsigned pFunc = getBlockAddress(&func);
        // if(pFunc != 0x80491d6) {
        //     continue;
        // }

        for (BasicBlock &bb : func) {
            if (!isRecoveredBlock(&bb)) {
                continue;
            }

            if (getBlockAddress(&bb) != pFunc) {
                continue;
            }

            // func.setLinkage(GlobalValue::InternalLinkage);
            // func.removeFnAttr(Attribute::AlwaysInline);

            DBG("[globalize_callback_addresses] inspecting callback: " << func.getName());
            ConstantInt *address = ConstantInt::get(i32Ty, pFunc);
            std::vector<Use*> work_list;

            for (Use &use : address->uses()) {
                auto call = dyn_cast<CallInst>(use.getUser());
                if (!call) {
                    // DBG(">> not a call: " << *use);
                    continue;
                }

                Function *target = call->getCalledFunction();
                if (!target) {
                    // DBG(">> no target function: " << *call);
                    continue;
                }

                if (target->getName() == "helper_stl_mmu") {
                    DBG("[globalize_callback_addresses] found potential function pointer: " << call);
                    work_list.push_back(&use);
                } else {
                    // DBG(">> not the helper function: " << *call);
                }
            }

            if (work_list.empty()) {
                continue;
            }

            auto global_var = new GlobalVariable(
                m,
                i32Ty,
                true,
                GlobalValue::ExternalLinkage,
                address,
                "Callback_" + utohexstr(pFunc));
            changed = true;

            DBG("[globalize_callback_addresses] creating global variable for callback address: " << *global_var);
            for(Use *use : work_list) {
                DBG("[globalize_callback_addresses] before: " << *use->getUser());
                // *use = global_var;
                Value *sym = b.CreateBitOrPointerCast(
                            global_var,
                            i32Ty);
                *use = sym;
                DBG("[globalize_callback_addresses] after: " << *use->getUser());
            }

            func.setLinkage(GlobalValue::InternalLinkage);
            func.removeFnAttr(Attribute::AlwaysInline);
            func.addFnAttr(Attribute::NoInline); 
        }
    }

    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}

}