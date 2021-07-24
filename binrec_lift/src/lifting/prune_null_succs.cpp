#include "prune_null_succs.hpp"
#include "ir/selectors.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto PruneNullSuccsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    std::vector<Function *> succs;

    for (Function &f : LiftedFunctions{m}) {
        MDNode *md;
        if ((md = getBlockMeta(&f, "succs"))) {
            bool update = false;
            succs.clear();

            for (unsigned i = 0, i_upper_bound = md->getNumOperands(); i < i_upper_bound; ++i) {
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
