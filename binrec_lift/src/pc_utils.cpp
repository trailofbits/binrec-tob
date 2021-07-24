#include "pc_utils.hpp"
#include "pass_utils.hpp"
#include <algorithm>
#include <llvm/IR/CFG.h>

using namespace llvm;

auto binrec::getConstPcOperand(Instruction *inst) -> unsigned
{
    if (isInstStart(inst)) {
        if (auto *con = dyn_cast<ConstantInt>(cast<StoreInst>(inst)->getValueOperand())) {
            return con->getZExtValue();
        }
    }
    return 0;
}

auto binrec::getFirstInstStart(BasicBlock *bb) -> StoreInst *
{
    for (Instruction &inst : *bb) {
        if (isInstStart(&inst))
            return cast<StoreInst>(&inst);
    }
    return nullptr;
}

auto binrec::getLastInstStart(BasicBlock *bb) -> StoreInst *
{
    StoreInst *last = nullptr;

    do {
        for (Instruction &inst : *bb) {
            if (isInstStart(&inst))
                last = cast<StoreInst>(&inst);
        }

        bb = succ_begin(bb) != succ_end(bb) ? *succ_begin(bb) : nullptr;
    } while (bb);

    return last;
}

auto binrec::findInstStart(BasicBlock *bb, unsigned pc) -> StoreInst *
{
    for (Instruction &inst : *bb) {
        if (getConstPcOperand(&inst) == pc)
            return cast<StoreInst>(&inst);
    }

    for (BasicBlock *succ : successors(bb)) {
        // XXX: would loop infinitely in cyclic CFG
        if (StoreInst *found = findInstStart(succ, pc))
            return found;
    }

    return nullptr;
}

auto binrec::findLastPCStore(BasicBlock &bb) -> StoreInst *
{
    for (auto inst = bb.rbegin(), end = bb.rend(); inst != end; ++inst) {
        if (auto *store = dyn_cast<StoreInst>(&*inst)) {
            return store;
        }
    }
    return nullptr;
}

auto binrec::getLastPc(BasicBlock *bb) -> unsigned
{
    MDNode *md = bb->getTerminator()->getMetadata("lastpc");
    assert(md && "expected lastpc in metadata node");
    return cast<ConstantInt>(dyn_cast<ValueAsMetadata>(md->getOperand(0))->getValue())
        ->getZExtValue();
}

auto binrec::getLastInstStartPc(BasicBlock *bb, bool allowEmpty, bool noRecovered) -> unsigned
{
    if (noRecovered && isRecoveredBlock(bb))
        return 0;

    unsigned lastPc = 0;

    for (Instruction &inst : *bb) {
        if (isInstStart(&inst))
            lastPc = getConstPcOperand(&inst);
    }

    for (BasicBlock *succ : successors(bb)) {
        lastPc = std::max(getLastInstStartPc(succ, allowEmpty, true), lastPc);
    }

    if (!noRecovered && !allowEmpty)
        assert(lastPc);

    return lastPc;
}

auto getPc(BasicBlock *bb) -> uint32_t
{
    std::string bbName(bb->getName());
    std::string::size_type s = bbName.find("BB_");
    if (s == std::string::npos)
        return 0;
    std::string bbPc(bbName.substr(s + 3, bbName.find(".from") - s - 3));
    uint32_t pc = std::stoi(bbPc, nullptr, 16);
    return pc;
}
