#include "internalize_imports.hpp"
#include "ir/selectors.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto InternalizeImportsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    m.setTargetTriple("i386-unknown-linux-gnu");

    for (Function &f : m) {
        if (f.isDeclaration() || is_lifted_function(f)) {
            continue;
        }

        // Prevent -globaldce from removing it since we will call it from main function
        if (f.getName() == "helper_fninit") {
            continue;
        }

        f.setLinkage(GlobalValue::InternalLinkage);
    }

    return PreservedAnalyses::all();
}
