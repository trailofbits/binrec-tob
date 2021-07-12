#include "RemoveOptNone.h"

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto RemoveOptNonePass::run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
    for (Function &f : m) {
        f.removeFnAttr(Attribute::OptimizeNone);
        f.removeFnAttr(Attribute::NoInline);
    }
    return PreservedAnalyses::none();
}
