#include "internalize_debug_helpers.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto InternalizeDebugHelpersPass::run(Function &f, FunctionAnalysisManager &am) -> PreservedAnalyses
{
    if (f.hasName() && (f.getName().startswith("mywrite") || f.getName().startswith("helper_"))) {
        f.setLinkage(GlobalValue::InternalLinkage);
    }
    return PreservedAnalyses::all();
}
