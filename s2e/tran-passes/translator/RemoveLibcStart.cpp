/*
 * remove libc setup/teardown code:
 * - remove all BBs whose addresses are not in the whitelisted main-addrs file
 */

#include <iostream>
#include <set>
#include <stdio.h>

#include "RemoveLibcStart.h"
#include "PassUtils.h"
#include "MetaUtils.h"
#include "PcUtils.h"

using namespace llvm;

char RemoveLibcStart::ID = 0;
static RegisterPass<RemoveLibcStart> X("remove-libc-start",
        "S2E Remove BB functions of libc header (anything before or after main function)",
        false, false);

static void moveEntryBlock(Function *f) {
    DBG("move " << f->getName() << " to module entry point");
    Module *m = f->getParent();
    f->removeFromParent();
    m->getFunctionList().push_front(f);
}

static void removeFromSuccessors(Function *f, Function *target) {
    std::vector<Function*> succs, newSuccs;
    getBlockSuccs(f, succs);
    succs.erase(std::remove(succs.begin(), succs.end(), target), succs.end());
    setBlockSuccs(f, succs);
}

static Function *findContainingBlock(Module &m, unsigned addr) {
    foreach(f, m) {
        if (!isRecoveredBlock(&*f))
            continue;

        unsigned bbStart = getBlockAddress(&*f);
        unsigned bbEnd = getLastPc(&*f);

        if (bbStart <= addr && addr <= bbEnd)
            return &*f;
    }

    return NULL;
}

bool RemoveLibcStart::runOnModule(Module &m) {
    std::ifstream f;
    s2eOpen(f, "main-addrs");

    std::set<unsigned> whitelist;
    unsigned addr;
    char flag;
    Function *entry = NULL;
    std::vector<Function*> exits;

    while (f >> std::hex >> addr) {
        whitelist.insert(addr);

        if (f.peek() == ' ') {
            f >> flag;
            assert(f.good());

            if (flag == 'e') {
                assert(!entry && "entry point already set");
                entry = m.getFunction("Func_" + intToHex(addr));
            } else if (flag == 'x') {
                if (Function *x = findContainingBlock(m, addr))
                    exits.push_back(x);
                else
                    DBG("could not find BB containing exit address " << addr);
            } else {
                assert(!"invalid flag");
            }
        }
    }

    // move entry block
    assert(entry);
    moveEntryBlock(entry);

    // remove entry block from successor lists
    foreach(f, m) {
        if (isRecoveredBlock(&*f))
            removeFromSuccessors(&*f, entry);
    }

    // remove successor lists of exit blocks
    //assert(exits.size() && "no exit points found");

    for (Function *f : exits) {
        DBG("clearing successor list of exit block " << f->getName());

        if (MDNode *succs = getBlockMeta(f, "succs"))
            setBlockMeta(f, "succs", NULL);
    }

#if 0
    // remove BBs whose address is not in the whitelist
    std::list<Function*> eraseList;

    foreach(f, m) {
        if (isRecoveredBlock(f) &&
                whitelist.find(getBlockAddress(f)) == whitelist.end()) {
            eraseList.push_back(f);
        }
    }

    foreach(it, eraseList) {
        DBG("removing non-main block " << (*it)->getName());
        (*it)->eraseFromParent();
    }
#endif

    // remove any remaining null references from metadata
    foreach(f, m) {
        if (isRecoveredBlock(&*f)) {
            if (MDNode *succs = getBlockMeta(&*f, "succs"))
                setBlockMeta(&*f, "succs", removeNullOperands(succs));
        }
    }

    return true;
}
