#include "RemoveMetadata.h"
#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char RemoveMetadata::ID = 0;
static RegisterPass<RemoveMetadata> X("remove-metadata",
        "S2E Remove all unused metadata if not in debug mode", false, false);

static void removeFromModule(Module &m, StringRef name) {
    if (NamedMDNode *md = m.getNamedMetadata(name))
        md->eraseFromParent();
}

bool RemoveMetadata::runOnModule(Module &m) {
    removeFromModule(m, "source_path");
    removeFromModule(m, "sections");

    for (Function &f : m) {
        for (BasicBlock &bb : f) {
            setBlockMeta(&bb, "extern_symbol", NULL);
            setBlockMeta(&bb, "succs", NULL);
            setBlockMeta(&bb, "lastpc", NULL);
            setBlockMeta(&bb, "merged", NULL);

            if (debugVerbosity == 0)
                setBlockMeta(&bb, "funcname", NULL);

            for (Instruction &i : bb) {
                i.setMetadata("srcloc", NULL);

                if (debugVerbosity == 0)
                    i.setMetadata("inststart", NULL);
            }
        }
    }

    return true;
}
