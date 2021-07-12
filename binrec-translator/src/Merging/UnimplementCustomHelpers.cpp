#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
void unimplementIfExists(Module &m, StringRef name) {
    if (Function *clone = m.getFunction(name)) {
        DBG("unimplementing helper " << name);
        clone->deleteBody();
    }
}

/// S2E Remove implementations for custom helpers (from custom-helpers.bc) to prepare for linking
class UnimplementCustomHelpersPass : public PassInfoMixin<UnimplementCustomHelpersPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        std::unique_ptr<llvm::Module> helpersbc = loadBitcodeFile(runlibDir() + "/custom-helpers.bc", m.getContext());

        for (Function &f : *helpersbc) {
            if (f.hasName()) {
                if (f.empty()) {
                    if (Function *clone = m.getFunction(f.getName()))
                        clone->setLinkage(GlobalValue::ExternalLinkage);
                } else {
                    unimplementIfExists(m, f.getName());
                }
            }
        }

        unimplementIfExists(m, "__ldb_mmu");
        unimplementIfExists(m, "__ldw_mmu");
        unimplementIfExists(m, "__ldl_mmu");
        unimplementIfExists(m, "__ldq_mmu");
        unimplementIfExists(m, "__stb_mmu");
        unimplementIfExists(m, "__stw_mmu");
        unimplementIfExists(m, "__stl_mmu");
        unimplementIfExists(m, "__stq_mmu");

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addUnimplementCustomHelpersPass(ModulePassManager &mpm) { mpm.addPass(UnimplementCustomHelpersPass()); }
