#include "unimplement_custom_helpers.hpp"
#include "pass_utils.hpp"
#include <regex>

using namespace binrec;
using namespace llvm;

static void unimplement_if_exists(Module &m, StringRef name)
{
    if (Function *clone = m.getFunction(name)) {
        DBG("unimplementing helper " << name);
        clone->deleteBody();
    }
}

// NOLINTNEXTLINE
auto UnimplementCustomHelpersPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    std::unique_ptr<llvm::Module> helpersbc =
        loadBitcodeFile(runlibDir() + "/custom-helpers.bc", m.getContext());

    for (Function &f : *helpersbc) {
        if (f.hasName()) {
            if (f.empty()) {
                if (Function *clone = m.getFunction(f.getName()))
                    clone->setLinkage(GlobalValue::ExternalLinkage);
            } else {
                unimplement_if_exists(m, f.getName());
            }
        }
    }

    // The ldl_mmu functions have new names, some are suffixed with a number
    // TODO (hbrodin): What does the suffix mean?
    std::regex re("^helper_(ld|st)[bwlq]_mmu(\\.\\d+)?$");
    for (Function &f : m) {
        if (!f.hasName())
            continue;
        auto name = f.getName();

        if (std::regex_match(name.begin(), name.end(), re))
            f.deleteBody();
    }


    return PreservedAnalyses::none();
}