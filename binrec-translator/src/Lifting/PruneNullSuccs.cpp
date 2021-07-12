#include "IR/Selectors.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

namespace {
/// S2E Remove any null entries from successor lists
///
/// remove libc setup/teardown code:
/// - remove all BBs whose addresses are not in the whitelisted main-addrs file
class PruneNullSuccsPass : public PassInfoMixin<PruneNullSuccsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        std::vector<Function *> succs;

        for (Function &f : LiftedFunctions{m}) {
            MDNode *md;
            if ((md = getBlockMeta(&f, "succs"))) {
                bool update = false;
                succs.clear();

                for (unsigned i = 0, iUpperBound = md->getNumOperands(); i < iUpperBound; ++i) {
                    if (const MDOperand &op = md->getOperand(i)) {
                        Value *value = cast<ValueAsMetadata>(op)->getValue();
                        succs.push_back(cast<Function>(value));
                    } else {
                        update = true;
                    }
                }

                if (update) {
                    setBlockSuccs(&f, succs);
                }
            }
        }

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addPruneNullSuccsPass(ModulePassManager &mpm) { mpm.addPass(PruneNullSuccsPass()); }
