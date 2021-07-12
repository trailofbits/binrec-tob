#include "AddCustomHelperVars.h"
#include "PassUtils.h"
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto AddCustomHelperVarsPass::run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
    Type *ty = Type::getInt32Ty(m.getContext());
    new GlobalVariable(m, ty, true, GlobalValue::ExternalLinkage, ConstantInt::get(ty, debugVerbosity),
                       "debug_verbosity");

    return PreservedAnalyses::all();
}
