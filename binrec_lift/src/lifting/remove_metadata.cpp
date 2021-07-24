#include "remove_metadata.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

static void remove_from_module(Module &m, StringRef name)
{
    if (NamedMDNode *md = m.getNamedMetadata(name))
        md->eraseFromParent();
}

// NOLINTNEXTLINE
auto RemoveMetadataPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    remove_from_module(m, "source_path");
    remove_from_module(m, "sections");

    for (Function &f : m) {
        for (BasicBlock &bb : f) {
            setBlockMeta(&bb, "extern_symbol", nullptr);
            setBlockMeta(&bb, "succs", nullptr);
            setBlockMeta(&bb, "lastpc", nullptr);
            setBlockMeta(&bb, "merged", nullptr);

            if (debugVerbosity == 0)
                setBlockMeta(&bb, "funcname", nullptr);

            for (Instruction &i : bb) {
                i.setMetadata("srcloc", nullptr);

                if (debugVerbosity == 0)
                    i.setMetadata("inststart", nullptr);
            }
        }
    }

    return PreservedAnalyses::all();
}
