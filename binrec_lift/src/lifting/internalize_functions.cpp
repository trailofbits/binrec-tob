#include "internalize_functions.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto InternalizeFunctionsPass::run(Function &f, FunctionAnalysisManager &am) -> PreservedAnalyses
{
    if (f.hasName() && f.getName() != "main" && !f.getName().startswith("mywrite") &&
        !f.getName().startswith("__stub") && f.getName() != "helper_stub_trampoline")
    {
        f.setLinkage(GlobalValue::InternalLinkage);
        return PreservedAnalyses::allInSet<CFGAnalyses>();
    }

    return PreservedAnalyses::all();
}
