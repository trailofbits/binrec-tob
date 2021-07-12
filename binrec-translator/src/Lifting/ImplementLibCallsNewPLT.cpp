#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
/// make a stub call into a full fledged call through the plt
class ImplementLibCallsNewPLTPass : public PassInfoMixin<ImplementLibCallsNewPLTPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        for (Function &f : m.getFunctionList()) {
            if (f.hasName() && f.getName().startswith("__stub")) {
                FunctionCallee newFc =
                    m.getOrInsertFunction(f.getName().drop_front(7), f.getFunctionType(), f.getAttributes());
                auto *newF = dyn_cast<Function>(newFc.getCallee());
                for (User *use : f.users()) {
                    if (auto *call = dyn_cast<CallInst>(use)) {
                        std::vector<Value *> args;
                        for (auto &arg : call->args()) {
                            args.push_back(arg);
                        }
                        IRBuilder<> b(call);
                        Value *ret = b.CreateCall(newF, args);
                        call->replaceAllUsesWith(ret);
                        call->eraseFromParent();
                    }
                }
            }
        }

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addImplementLibCallsNewPLTPass(ModulePassManager &mpm) { mpm.addPass(ImplementLibCallsNewPLTPass()); }
