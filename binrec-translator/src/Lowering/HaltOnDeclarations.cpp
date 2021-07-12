#include "DebugUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include <algorithm>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/PassManager.h>

using namespace llvm;

namespace {
/// S2E List any function declarations whose implementation is missing
class HaltOnDeclarationsPass : public PassInfoMixin<HaltOnDeclarationsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        bool halt = false;

        std::vector<Function *> intrinsics;

        for (Function &f : m) {
            if (f.empty() && f.getNumUses() > 0 && f.hasName() && !f.getName().startswith("__stub")) {
                if (f.isIntrinsic()) {
                    WARNING("intrinsic " << f.getName() << " may create PLT entries (used " << f.getNumUses()
                                         << " times)");
                    intrinsics.push_back(&f);
                } /* else {
                    ERROR("missing implementation for " << f.getName() << " (used " << f.getNumUses() << " times)");
                    halt = true;
                } */
            }
        }

        if (halt) {
            errs() << "found function declarations, exiting with error status\n";
            exit(1);
        }

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addHaltOnDeclarationsPass(ModulePassManager &mpm) { mpm.addPass(HaltOnDeclarationsPass()); }
