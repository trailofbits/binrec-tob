#include "rename_block_funcs.hpp"
#include "ir/selectors.hpp"
#include "pass_utils.hpp"
#include "binrec/tracing/trace_info.hpp"
#include <fstream>
#include <set>

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto RenameBlockFuncsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    unsigned removed = 0, total = 0;

    // Find all distinct basic block addresses that should be in the CFG
    TraceInfo ti;
    {
        std::ifstream f;
        s2eOpen(f, TraceInfo::defaultFilename);
        f >> ti;
    }
    std::set<uint32_t> known_pcs;
    for (auto successor : ti.successors) {
        known_pcs.insert(successor.pc);
        known_pcs.insert(successor.successor);
    }

    DBG("read " << known_pcs.size() << " block addresses from traceInfo.json");

    // To support runs that did renaming at runtime, ignore any PCs that
    // already have a function with a Func_ prefix
    for (Function &f : LiftedFunctions{m}) {
        known_pcs.erase(getBlockAddress(&f));
        total++;
    }

    // For all captured blocks, either rename to Func_ prefix (if no such
    // function exists yet) or remove the unused block
    std::list<Function *> erase_list;
    std::set<uint32_t> renamed_pcs;

    for (Function &f : m) {
        if (f.hasName() && f.getName().startswith("tcg-llvm-")) {
            unsigned address = hexToInt(f.getName().substr(f.getName().rfind('-') + 1));

            if (known_pcs.find(address) == known_pcs.end() ||
                renamed_pcs.find(address) != renamed_pcs.end()) {
                erase_list.push_back(&f);
                removed++;
            } else {
                f.setName("Func_" + utohexstr(address));
                renamed_pcs.insert(address);
            }
        }
        // Also fix any broken declarations with private linkage, which S2E
        // creates for some reason
        else if (f.isDeclaration() && f.getLinkage() == GlobalValue::PrivateLinkage)
        {
            f.setLinkage(GlobalValue::ExternalLinkage);
        }
    }

    total += renamed_pcs.size();

    for (Function *f : erase_list)
        f->eraseFromParent();

    INFO(
        "renamed " << renamed_pcs.size() << " blocks, removed " << removed << " blocks, " << total
                   << " blocks remaining");

    assert(renamed_pcs.size() <= known_pcs.size());

    return PreservedAnalyses::none();
}
