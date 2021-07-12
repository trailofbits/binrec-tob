#include "MetaUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

namespace {
void removeFromModule(Module &m, StringRef name) {
    if (NamedMDNode *md = m.getNamedMetadata(name))
        md->eraseFromParent();
}

/// S2E Remove all unused metadata if not in debug mode
class RemoveMetadataPass : public PassInfoMixin<RemoveMetadataPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        removeFromModule(m, "source_path");
        removeFromModule(m, "sections");

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
};
} // namespace

void binrec::addRemoveMetadataPass(ModulePassManager &mpm) { mpm.addPass(RemoveMetadataPass()); }
