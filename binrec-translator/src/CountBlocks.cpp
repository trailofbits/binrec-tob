#include "CountBlocks.h"
#include "IR/Selectors.h"
#include "PassUtils.h"
#include <climits>
#include <cstring>
#include <fstream>
#include <set>
#include <unistd.h>

using namespace binrec;
using namespace llvm;

char CountBlocks::ID = 0;
static RegisterPass<CountBlocks> x("count-blocks", "S2E Count number of recovered blocks from bblist", false, false);

auto CountBlocks::runOnModule(Module &m) -> bool {
    char bbListPath[PATH_MAX] = "";

    if (readlink(s2eOutFile("binary").c_str(), bbListPath, sizeof(bbListPath)) < 0) {
        perror("readlink");
        exit(1);
    }

    strncat(bbListPath, ".bblist", sizeof(bbListPath) - strlen(bbListPath) - 1);

    std::ifstream f;
    std::set<uint32_t> knownPcs;
    uint32_t addr = 0;

    fileOpen(f, bbListPath);

    while (f.read((char *)&addr, sizeof(uint32_t)))
        knownPcs.insert(addr);

    f.close();

    DBG("read " << knownPcs.size() << " block addresses from " << bbListPath);

    unsigned count = 0;

    for (Function &f : LiftedFunctions{m}) {
        if (knownPcs.find(getBlockAddress(&f)) != knownPcs.end())
            count++;
    }

    INFO("blocks encountered from bblist: " << count);

    return false;
}
