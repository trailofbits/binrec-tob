#include "llvm/IR/DebugInfo.h"

#include "UnimplementCustomHelpers.h"
#include "PassUtils.h"

using namespace llvm;

char UnimplementCustomHelpers::ID = 0;
static RegisterPass<UnimplementCustomHelpers> x("unimplement-custom-helpers",
        "S2E Remove implementations for custom helpers (from custom-helpers.bc) to prepare for linking",
        false, false);

static void unimplementIfExists(Module &m, StringRef name) {
    if (Function *clone = m.getFunction(name)) {
        DBG("unimplementing helper " << name);
        clone->deleteBody();
    }
}

bool UnimplementCustomHelpers::runOnModule(Module &m) {
    std::unique_ptr<llvm::Module> helpersbc = loadBitcodeFile(runlibDir() + "/custom-helpers.bc");

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

    return true;
}
