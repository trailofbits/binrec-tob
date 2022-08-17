#ifndef BINREC_DEBUG_PRINT_CALLBACK_HPP
#define BINREC_DEBUG_PRINT_CALLBACK_HPP

#define FUNC_NAME "Func_80491B6"

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "ir/selectors.hpp"

namespace binrec {
    class DebugPrintCallback : public llvm::PassInfoMixin<DebugPrintCallback> {
    public:
        llvm::StringRef pass_name_;
        bool print_func_;
        DebugPrintCallback(llvm::StringRef pass_name, bool print_func = false) : llvm::PassInfoMixin<DebugPrintCallback>(), pass_name_(pass_name), print_func_(print_func) {}
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses {
            auto func = m.getFunction(FUNC_NAME);
            if(func) {
                DBG("[debug_print_callback] " << pass_name_ << ": found");
                if (!func->hasFnAttribute(llvm::Attribute::NoInline)) {
                    DBG("[debug_print_callback] adding noline attribute");
                    func->addFnAttr(llvm::Attribute::NoInline);
                }
                if (print_func_) {
                    DBG(*func);
                }
            } else {
                DBG("[debug_print_callback] " << pass_name_ << ": !!! NOT FOUND");
            }

            return llvm::PreservedAnalyses::all();
        }
    };
} // namespace binrec

#endif
