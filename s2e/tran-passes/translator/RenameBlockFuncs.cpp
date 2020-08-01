#include <set>
#include "RenameBlockFuncs.h"
#include "PassUtils.h"

using namespace llvm;

char RenameBlockFuncs::ID = 0;
static RegisterPass<RenameBlockFuncs> x("rename-block-funcs",
        "S2E Rename captured basic blocks to Func_ prefix and remove unused blocks",
        false, false);

bool RenameBlockFuncs::runOnModule(Module &m) {
    unsigned removed = 0, total = 0;

    // Find all distinct basic block addresses that should be in the CFG
    std::ifstream f;
    uint32_t pred, succ;

    s2eOpen(f, "succs.dat");
    std::set<uint32_t> knownPcs;

    while (f.read((char*)&succ, 4).read((char*)&pred, 4)) {
        knownPcs.insert(pred);
        knownPcs.insert(succ);
    }

    DBG("read " << knownPcs.size() << " block addresses from succs.dat");

    // To support runs that did renaming at runtime, ignore any PCs that
    // already have a function with a Func_ prefix
    for (Function &f : m) {
        if (isRecoveredBlock(&f)) {
            knownPcs.erase(getBlockAddress(&f));
            total++;
        }
    }

    // For all captured blocks, either rename to Func_ prefix (if no such
    // function exists yet) or remove the unused block
    std::list<Function*> eraseList;
    std::set<uint32_t> renamedPcs;

    for (Function &f : m) {
        if (f.hasName() && f.getName().startswith("tcg-llvm-tb-")) {
            unsigned address = hexToInt(f.getName().substr(f.getName().rfind('-') + 1));

            if (knownPcs.find(address) == knownPcs.end() ||
                    renamedPcs.find(address) != renamedPcs.end()) {
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

    INFO("renamed " << renamedPcs.size() << " blocks, removed " << removed <<
            " blocks, " << total << " blocks remaining");

    assert(renamedPcs.size() <= knownPcs.size());

    return true;
}
