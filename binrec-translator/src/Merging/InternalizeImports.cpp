#include "IR/Selectors.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

namespace {
/// S2E Internalize non-recovered functions for -globaldce
class InternalizeImportsPass : public PassInfoMixin<InternalizeImportsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        m.setTargetTriple("i386-unknown-linux-gnu");

        for (Function &f : m) {
            if (f.isDeclaration() || isLiftedFunction(f)) {
                continue;
            }

            // Prevent -globaldce from removing it since we will call it from main function
            if (f.getName() == "helper_fninit") {
                continue;
            }

            f.setLinkage(GlobalValue::InternalLinkage);
        }

        return PreservedAnalyses::all();
    }
};
} // namespace

void binrec::addInternalizeImportsPass(ModulePassManager &mpm) { mpm.addPass(InternalizeImportsPass()); }
