#include "CountInsts.h"
#include "IR/Selectors.h"
#include "PassUtils.h"
#include <climits>
#include <cstring>
#include <set>
#include <unistd.h>

using namespace llvm;

char CountInsts::ID = 0;
static RegisterPass<CountInsts> x("count-insts", "S2E Count number of recovered instructions", false, false);

static void countInWrapper(Function &wrapper) {
    unsigned recbbs = 0, otherbbs = 0, insts = 0;

    for (BasicBlock &bb : wrapper) {
        if (isRecoveredBlock(&bb))
            recbbs++;
        else
            otherbbs++;

        insts += bb.size();
    }

    INFO("counted " << insts << " instructions in " << recbbs << " recovered blocks (" << recbbs + otherbbs
                    << " total) in @wrapper");
}

static void countInFunctionBlocks(Module &m) {
    unsigned bbs = 0, insts = 0;

    for (Function &f : binrec::LiftedFunctions{m}) {
        bbs++;

        for (BasicBlock &bb : f)
            insts += bb.size();
    }

    INFO("counted " << insts << " instructions in " << bbs << " recovered block functions");
}

static void countInMain(Function &main) {
    unsigned bbs = 0, insts = 0;

    for (BasicBlock &bb : main) {
        bbs++;
        insts += bb.size();
    }

    INFO("counted " << insts << " instructions in " << bbs << " basic blocks in @main");
}

auto CountInsts::runOnModule(Module &m) -> bool {
    if (Function *wrapper = m.getFunction("Func_wrapper"))
        countInWrapper(*wrapper);
    else
        countInFunctionBlocks(m);

    if (Function *main = m.getFunction("main"))
        countInMain(*main);

    return false;
}
