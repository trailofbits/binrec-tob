#include "implement_lib_calls_new_plt.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;
using namespace std;

// NOLINTNEXTLINE
auto ImplementLibCallsNewPLTPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    for (Function &f : m.getFunctionList()) {
        if (f.hasName() && f.getName().startswith("__stub")) {
            FunctionCallee new_fc = m.getOrInsertFunction(
                f.getName().drop_front(7),
                f.getFunctionType(),
                f.getAttributes());

            INFO("replacing calls to stub: " << f.getName());
            vector<CallInst *> work_list;
            for (User *user : f.users()) {
                auto call = dyn_cast<CallInst>(user);
                if (call) {
                    work_list.push_back(call);
                }
            }

            for (CallInst *call : work_list) {
                std::vector<Value *> args;
                for (auto &arg : call->args()) {
                    args.push_back(arg);
                }
                IRBuilder<> b(call);
                Value *ret = b.CreateCall(new_fc, args);
                call->replaceAllUsesWith(ret);
                call->eraseFromParent();
            }
        }
    }

    return PreservedAnalyses::none();
}
