#include <algorithm>

#include "PassUtils.h"
#include "PcUtils.h"

using namespace llvm;

unsigned getConstPcOperand(Instruction *inst) {
    if (isInstStart(inst)) {
        if (ConstantInt *con = dyn_cast<ConstantInt>(
                    cast<StoreInst>(inst)->getValueOperand())) {
            return con->getZExtValue();
        }
    }
    return 0;
}

StoreInst *getFirstInstStart(BasicBlock *bb) {
    for(Instruction &inst : *bb) {
        if (isInstStart(&inst))
            return cast<StoreInst>(&inst);
    }
    return NULL;
}

StoreInst *getLastInstStart(BasicBlock *bb) {
    if (!isRecoveredBlock(bb))
        return NULL;

    StoreInst *last = NULL;

    do {
        for (Instruction &inst : *bb) {
            if (isInstStart(&inst))
                last = cast<StoreInst>(&inst);
        }

        bb = succ_begin(bb) != succ_end(bb) ? *succ_begin(bb) : NULL;
    } while (bb);

    return last;
}

StoreInst *findInstStart(BasicBlock *bb, unsigned pc) {
    for(Instruction &inst : *bb) {
        if (getConstPcOperand(&inst) == pc)
            return cast<StoreInst>(&inst);
    }

    foreach2(it, succ_begin(bb), succ_end(bb)) {
        // XXX: would loop infinitely in cyclic CFG
        if (StoreInst *found = findInstStart(*it, pc))
            return found;
    }

    return NULL;
}

unsigned getLastPc(BasicBlock *bb) {
    MDNode *md = bb->getTerminator()->getMetadata("lastpc");
    assert(md && "expected lastpc in metadata node");
    return cast<ConstantInt>(dyn_cast<ValueAsMetadata>(md->getOperand(0))->getValue())->getZExtValue();
}

unsigned getLastInstStartPc(BasicBlock *bb, bool allowEmpty, bool noRecovered) {
    if (noRecovered && isRecoveredBlock(bb))
        return 0;

    unsigned lastpc = 0;

    for (Instruction &inst : *bb) {
        if (isInstStart(&inst))
            lastpc = getConstPcOperand(&inst);
    }

    foreach2(it, succ_begin(bb), succ_end(bb))
        lastpc = std::max(getLastInstStartPc(*it, allowEmpty, true), lastpc);

    if (!noRecovered && !allowEmpty)
        assert(lastpc);

    return lastpc;
}
