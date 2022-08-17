#include "replace_local_function_pointers.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "ir/selectors.hpp"
#include <algorithm>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

using namespace llvm;

namespace binrec {

auto ReplaceLocalFunctionPointers::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses {
    bool changed = false;
    IntegerType *i32Ty = IntegerType::getInt32Ty(m.getContext());
    IRBuilder<> builder(m.getContext());
    std::vector<GlobalVariable*> work_list;

    for (GlobalVariable& global_var : m.getGlobalList()) {
        if (!global_var.getName().startswith("Callback_")) {
            continue;
        }

        work_list.push_back(&global_var);
    }

    for (GlobalVariable *callback_var : work_list) {
        std::string func_name{"Func_"};
        func_name += callback_var->getName().substr(9);
        DBG("[replace_local_function_pointers] inspecting global: " << callback_var->getName());
        Function *func = m.getFunction(func_name);
        if (!func) {
            DBG("[replace_local_function_pointers] function does not exist: " << func_name);
            continue;
        }

        for (Use &use : callback_var->uses()) {
            // auto inst = dyn_cast<Instruction>(use.getUser());
            // if (!inst) {
            //     // TODO
            //     continue;
            // }
            // builder.SetInsertPoint(inst);
            // Value *ptr = builder.CreateAlloca(func->getType());
            // builder.CreateStore(func, ptr, false);
            // llvm::Value *temp = builder.CreateLoad(i32Ty, ptr);
            // use = temp;
            auto cast = ConstantExpr::getBitCast(func, i32Ty);
            DBG("[replace_local_function_pointers] replacing use: " << use);
            use = cast;
            DBG("[replace_local_function_pointers] done: " << *use.getUser());
            changed = true;
        }

        callback_var->eraseFromParent();
    }

    return changed ? PreservedAnalyses::none() : PreservedAnalyses::all();
}
}
