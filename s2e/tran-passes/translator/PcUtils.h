#ifndef _PCUTILS_H
#define _PCUTILS_H

using namespace llvm;

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

unsigned getConstPcOperand(Instruction *inst);
StoreInst *getFirstInstStart(BasicBlock *bb);
StoreInst *getLastInstStart(BasicBlock *bb);
StoreInst *findInstStart(BasicBlock *bb, unsigned pc);
unsigned getLastPc(BasicBlock *bb);
unsigned getLastInstStartPc(BasicBlock *bb, bool allowEmpty = false, bool noRecovered = false);

static inline bool isInstStart(Instruction *inst) {
    return inst->getMetadata("inststart");
}

static inline unsigned getLastPc(Function *f) {
    return getLastPc(&f->getEntryBlock());
}

template<typename T>
static bool blocksOverlap(T *a, T *b) {
    unsigned aStart = getBlockAddress(a);
    unsigned aEnd = getLastPc(a);
    unsigned bStart = getBlockAddress(b);
    unsigned bEnd = getLastPc(b);
    return bStart > aStart && bEnd == aEnd;
}

template<typename T>
static inline bool haveSameEndAddress(T *a, T *b) {
    return getLastPc(a) == getLastPc(b);
}

template<typename T>
static inline bool blockContains(T *bb, unsigned pc) {
    return pc >= getBlockAddress(bb) && pc <= getLastPc(bb);
}

#endif  // _PCUTILS_H
