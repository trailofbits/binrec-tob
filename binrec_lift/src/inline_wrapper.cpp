#include "inline_wrapper.hpp"
#include "pass_utils.hpp"

using namespace binrec;
using namespace llvm;
using namespace std;

static void run_impl(Module &m)
{
    vector<StringRef> func_names{"main", "myexit", "helper_stub_trampoline", "helper_debug_state"};
    for (Function &f : m) {
        if (f.getName().startswith("llvm") || f.getName().startswith("mywrite") ||
            f.getName().startswith("__stub_") ||
            find(func_names.begin(), func_names.end(), f.getName()) != func_names.end() ||
            f.hasFnAttribute(Attribute::NoInline))
        {
            continue;
        }
        // errs() << "inline: " << f.getName() << "\n";
        f.addFnAttr(Attribute::AlwaysInline);
    }
}

// NOLINTNEXTLINE
auto InlineWrapperPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    run_impl(m);
    return PreservedAnalyses::none();
}

char InlineWrapperLegacyPass::ID = 0;
static RegisterPass<InlineWrapperLegacyPass>
    x("inline-wrapper",
      "S2E Tag @wrapper as always-inline (use in combination with -always-inline)",
      false,
      false);

auto InlineWrapperLegacyPass::runOnModule(Module &m) -> bool
{
    run_impl(m);
    return true;
}
