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
    //
    // We set these functions to external linkage because they are optimized to a
    // direct "load" instruction during lifting (see add_mem_array.cpp).
    std::regex mmu_re("^helper_(ld|st)[bwlq]_mmu(\\.\\d+)?$");
    std::regex cpu_re("^cpu_(ld|st)[a-z]+_data$");
    for (Function &f : m) {
        if (!f.hasName())
            continue;
        auto name = f.getName();

        bool matches =
            (std::regex_match(name.begin(), name.end(), mmu_re) ||
             std::regex_match(name.begin(), name.end(), cpu_re));
        if (matches) {
            DBG("deleting helper body: " << name);
            f.deleteBody();
        }
    }

    return PreservedAnalyses::none();
}
