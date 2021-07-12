#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
/// S2E Internalize functions for removal by -globalde
class InternalizeFunctionsPass : public PassInfoMixin<InternalizeFunctionsPass> {
public:
    auto run(Function &f, FunctionAnalysisManager &) -> PreservedAnalyses {
        if (f.hasName() && f.getName() != "main" && !f.getName().startswith("mywrite") &&
            !f.getName().startswith("__stub") && f.getName() != "helper_stub_trampoline") {
            // TODO(FPar): helper_stub_trampoline is optimized wrong sometimes (LLVM Bug?)
            // When running the function simplification pipeline after this and then `opt -O1` and there is only a
            // single call to this helper (which means that arguments can be inlined), then LLVM seems to incorrectly
            // replace all arguments to the inline assembly with 0. To prevent inlining arguments, have this function
            // link externally.
            f.setLinkage(GlobalValue::InternalLinkage);
            return PreservedAnalyses::allInSet<CFGAnalyses>();
        }

        return PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addInternalizeFunctionsPass(ModulePassManager &mpm) {
    return mpm.addPass(createModuleToFunctionPassAdaptor(InternalizeFunctionsPass()));
}
