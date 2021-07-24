#include "externalize_functions.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto ExternalizeFunctionsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    for (Function &f : m)
        f.setLinkage(GlobalValue::ExternalLinkage);
    return PreservedAnalyses::all();
}
