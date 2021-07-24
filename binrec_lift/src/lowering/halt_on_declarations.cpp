#include "halt_on_declarations.hpp"
#include "pass_utils.hpp"
#include <algorithm>
#include <llvm/IR/InlineAsm.h>

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto HaltOnDeclarationsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    std::vector<Function *> intrinsics;

    for (Function &f : m) {
        if (f.empty() && f.getNumUses() > 0 && f.hasName() && !f.getName().startswith("__stub")) {
            if (f.isIntrinsic()) {
                WARNING(
                    "intrinsic " << f.getName() << " may create PLT entries (used "
                                 << f.getNumUses() << " times)");
                intrinsics.push_back(&f);
            }
        }
    }

    return PreservedAnalyses::none();
}
