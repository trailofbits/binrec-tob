/*
 * remove libc setup/teardown code:
 * - remove all BBs whose addresses are not in the whitelisted main-addrs file
 */

#include <iostream>
#include <set>
#include <stdio.h>

#include "PruneNullSuccs.h"
#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char PruneNullSuccs::ID = 0;
static RegisterPass<PruneNullSuccs> X("prune-null-succs",
        "S2E Remove any null entries from successor lists",
        false, false);

bool PruneNullSuccs::runOnModule(Module &m) {
    MDNode *md;
    std::vector<Function*> succs;

    for(Function &f : m) {
        if (isRecoveredBlock(&f) && (md = getBlockMeta(&f, "succs"))) {
            bool update = false;
            succs.clear();

            fori(i, 0, md->getNumOperands()) {
                if (const MDOperand &op = md->getOperand(i))
                    succs.push_back(cast<Function>(dyn_cast<ValueAsMetadata>(op)->getValue()));
                else
                    update = true;
            }

            if (update)
                setBlockSuccs(&f, succs);
        }
    }

    return true;
}
