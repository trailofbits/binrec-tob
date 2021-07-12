#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
/// S2E Give all function definitions and declarations external linkage
class ExternalizeFunctionsPass : public PassInfoMixin<ExternalizeFunctionsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        for (Function &f : m)
            f.setLinkage(GlobalValue::ExternalLinkage);

        return PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addExternalizeFunctionsPass(llvm::ModulePassManager &mpm) { mpm.addPass(ExternalizeFunctionsPass()); }
