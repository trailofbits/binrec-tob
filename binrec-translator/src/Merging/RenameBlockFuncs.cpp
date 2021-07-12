#include "IR/Selectors.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include "binrec/tracing/TraceInfo.h"
#include <algorithm>
#include <fstream>
#include <llvm/IR/PassManager.h>
#include <set>

using namespace binrec;
using namespace llvm;

namespace {
/// S2E Rename captured basic blocks to Func_ prefix and remove unused blocks
class RenameBlockFuncsPass : public PassInfoMixin<RenameBlockFuncsPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        unsigned removed = 0, total = 0;

        // Find all distinct basic block addresses that should be in the CFG
        TraceInfo ti;
        {
            std::ifstream f;
            s2eOpen(f, TraceInfo::defaultFilename);
            f >> ti;
        }
        std::set<uint32_t> knownPcs;
        for (auto successor : ti.successors) {
            knownPcs.insert(successor.pc);
            knownPcs.insert(successor.successor);
        }

        DBG("read " << knownPcs.size() << " block addresses from traceInfo.json");

        // To support runs that did renaming at runtime, ignore any PCs that
        // already have a function with a Func_ prefix
        for (Function &f : LiftedFunctions{m}) {
            knownPcs.erase(getBlockAddress(&f));
            total++;
        }

        // For all captured blocks, either rename to Func_ prefix (if no such
        // function exists yet) or remove the unused block
        std::list<Function *> eraseList;
        std::set<uint32_t> renamedPcs;

        for (Function &f : m) {
            if (f.hasName() && f.getName().startswith("tcg-llvm-tb-")) {
                unsigned address = hexToInt(f.getName().substr(f.getName().rfind('-') + 1));

                if (knownPcs.find(address) == knownPcs.end() || renamedPcs.find(address) != renamedPcs.end()) {
                    eraseList.push_back(&f);
                    removed++;
                } else {
                    f.setName("Func_" + intToHex(address));
                    renamedPcs.insert(address);
                }
            }
            // Also fix any broken declarations with private linkage, which S2E
            // creates for some reason
            else if (f.isDeclaration() && f.getLinkage() == GlobalValue::PrivateLinkage) {
                f.setLinkage(GlobalValue::ExternalLinkage);
            }
        }

        total += renamedPcs.size();

        for (Function *f : eraseList)
            f->eraseFromParent();

        INFO("renamed " << renamedPcs.size() << " blocks, removed " << removed << " blocks, " << total
                        << " blocks remaining");

        assert(renamedPcs.size() <= knownPcs.size());

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addRenameBlockFuncsPass(ModulePassManager &mpm) { mpm.addPass(RenameBlockFuncsPass()); }
