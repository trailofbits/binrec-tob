#include "SuccessorLists.h"
#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char SuccessorLists::ID = 0;
static RegisterPass<SuccessorLists> x("successor-lists",
        "S2E Create metadata annotations for successor lists", false, false);

Function *SuccessorLists::getCachedFunc(Module &m, uint32_t pc) {
    if (bbCache.find(pc) == bbCache.end())
        bbCache[pc] = m.getFunction("Func_" + intToHex(pc));

    return bbCache[pc];
}

bool SuccessorLists::runOnModule(Module &m) {
    std::ifstream f;
    uint32_t pred, succ;
    s2eOpen(f, "succs.dat");

    while (f.read((char*)&succ, 4).read((char*)&pred, 4)) {
        Function *predBB = getCachedFunc(m, pred);

        if (!predBB) {
            // A block can be removed manually from the bitcode file
            continue;
        }

        Function *succBB = getCachedFunc(m, succ);

        std::vector<Function*> succs;
        getBlockSuccs(predBB, succs);
        succs.push_back(succBB);
        setBlockSuccs(predBB, succs);
   }

    return true;
}
