#include "add_custom_helper_vars.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto AddCustomHelperVarsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    Type *ty = Type::getInt32Ty(m.getContext());
    new GlobalVariable(
        m,
        ty,
        true,
        GlobalValue::ExternalLinkage,
        ConstantInt::get(ty, debugVerbosity),
        "debug_verbosity");

    return PreservedAnalyses::all();
}
