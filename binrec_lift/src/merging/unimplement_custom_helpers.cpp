#include "unimplement_custom_helpers.hpp"
#include "pass_utils.hpp"

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

    unimplement_if_exists(m, "__ldb_mmu");
    unimplement_if_exists(m, "__ldw_mmu");
    unimplement_if_exists(m, "__ldl_mmu");
    unimplement_if_exists(m, "__ldq_mmu");
    unimplement_if_exists(m, "__stb_mmu");
    unimplement_if_exists(m, "__stw_mmu");
    unimplement_if_exists(m, "__stl_mmu");
    unimplement_if_exists(m, "__stq_mmu");

    return PreservedAnalyses::none();
}