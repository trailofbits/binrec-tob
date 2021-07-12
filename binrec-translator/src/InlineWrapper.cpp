#include "InlineWrapper.h"
#include "PassUtils.h"

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
void runImpl(Module &m) {
    vector<StringRef> funcNames{"main", "myexit", "helper_stub_trampoline", "helper_debug_state"};
    for (Function &f : m) {
        if (f.getName().startswith("llvm") || f.getName().startswith("mywrite") || f.getName().startswith("__stub_") ||
            find(funcNames.begin(), funcNames.end(), f.getName()) != funcNames.end())
            continue;
        errs() << "inline: " << f.getName() << "\n";
        f.addFnAttr(Attribute::AlwaysInline);
    }
}
} // namespace

// NOLINTNEXTLINE
auto InlineWrapperPass::run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
    runImpl(m);
    return PreservedAnalyses::none();
}

char InlineWrapperLegacyPass::ID = 0;
static RegisterPass<InlineWrapperLegacyPass>
    x("inline-wrapper", "S2E Tag @wrapper as always-inline (use in combination with -always-inline)", false, false);

auto InlineWrapperLegacyPass::runOnModule(Module &m) -> bool {
    runImpl(m);
    return true;
}