#include "internalize_imports.hpp"
#include "ir/selectors.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;

// These qemu functions are used in custom-helpers.bc and must always be available.
static std::vector<StringRef> Ignore_Functions{
    "helper_fninit",
    "helper_fldl_ST0",
    "helper_flds_ST0"};

// NOLINTNEXTLINE
auto InternalizeImportsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    m.setTargetTriple("i386-unknown-linux-gnu");

    for (Function &f : m) {
        if (f.isDeclaration() || is_lifted_function(f)) {
            continue;
        }

        if (std::find(Ignore_Functions.begin(), Ignore_Functions.end(), f.getName()) !=
            Ignore_Functions.end())
        {
            // Prevent -globaldce from removing these functions
            continue;
        }

        DBG("setting internal linkage for function: " << f.getName());
        f.setLinkage(GlobalValue::InternalLinkage);
    }

    return PreservedAnalyses::all();
}
