#include "remove_opt_none.hpp"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto RemoveOptNonePass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    for (Function &f : m) {
        f.removeFnAttr(Attribute::OptimizeNone);
        f.removeFnAttr(Attribute::NoInline);
    }
    return PreservedAnalyses::none();
}
