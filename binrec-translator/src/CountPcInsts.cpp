#include "CountPcInsts.h"
#include "MetaUtils.h"
#include "PassUtils.h"

using namespace llvm;

char CountPcInsts::ID = 0;
static RegisterPass<CountPcInsts>
    X("count-pc-inst", "S2E Count number of instructions that corresponds real instruction in binary", false, false);

static auto getNOfPcInst(BasicBlock &bb) -> unsigned {
    unsigned pcInsts = 0;

    for (Instruction &inst : bb) {
        if (inst.getMetadata("inststart")) {
            ++pcInsts;
        }
    }

    return pcInsts;
}

auto CountPcInsts::runOnModule(Module &m) -> bool {

    unsigned totalPcInsts = 0;
    if (Function *wrapper = m.getFunction("Func_wrapper")) {
        for (BasicBlock &B : *wrapper) {
            totalPcInsts += getNOfPcInst(B);
        }
    } else {
        for (Function &F : m) {
            for (BasicBlock &B : F) {
                totalPcInsts += getNOfPcInst(B);
            }
        }
    }

    errs() << "Total instruction coverage: " << totalPcInsts << '\n';
    return false;
}
