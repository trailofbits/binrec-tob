#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
/// S2E Internalize debug helper functions for removal by -globalde
class InternalizeDebugHelpersPass : public PassInfoMixin<InternalizeDebugHelpersPass> {
public:
    auto run(Function &f, FunctionAnalysisManager &) -> PreservedAnalyses {
        if (f.hasName() && (f.getName().startswith("mywrite") || f.getName().startswith("helper_"))) {
            f.setLinkage(GlobalValue::InternalLinkage);
        }
        return PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addInternalizeDebugHelpersPass(ModulePassManager &mpm) {
    mpm.addPass(createModuleToFunctionPassAdaptor(InternalizeDebugHelpersPass()));
}
