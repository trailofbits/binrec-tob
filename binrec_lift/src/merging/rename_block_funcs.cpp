#include "rename_block_funcs.hpp"
#include "error.hpp"
#include "ir/selectors.hpp"
#include "pass_utils.hpp"
#include "binrec/tracing/trace_info.hpp"
#include <fstream>
#include <set>

#define PASS_NAME "rename_block_funcs"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto RenameBlockFuncsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    unsigned removed = 0, total = 0;

    // Find all distinct basic block addresses that should be in the CFG
    TraceInfo ti;
    {
        // Can't use the default traceinfo filename here as this pass runs pre-link.
        // Symbolic traces that are numbered have correspondingly name trace info files.
        std::string modId = m.getModuleIdentifier();
        std::string filename;
        std::size_t pos = modId.find("_", modId.find_last_of("/"));
        if (pos == std::string::npos) {
            filename = TraceInfo::defaultFilename;
        } else {
            filename = TraceInfo::defaultName + modId.substr(pos, 2) + TraceInfo::defaultSuffix;
        }

        std::ifstream f;
        s2eOpen(f, filename);
        f >> ti;
    }
    std::set<uint32_t> known_pcs;
    for (auto successor : ti.successors) {
        known_pcs.insert(successor.pc);
        known_pcs.insert(successor.successor);
    }

    DBG("read " << known_pcs.size() << " block addresses from Trace Info file.");

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

    PASS_ASSERT(renamed_pcs.size() <= known_pcs.size());

    return PreservedAnalyses::none();
}
