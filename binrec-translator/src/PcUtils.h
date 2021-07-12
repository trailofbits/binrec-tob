#ifndef BINREC_PCUTILS_H
#define BINREC_PCUTILS_H

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

namespace binrec {
auto getConstPcOperand(llvm::Instruction *inst) -> unsigned;
auto getFirstInstStart(llvm::BasicBlock *bb) -> llvm::StoreInst *;
auto getLastInstStart(llvm::BasicBlock *bb) -> llvm::StoreInst *;
auto findInstStart(llvm::BasicBlock *bb, unsigned pc) -> llvm::StoreInst *;
auto findLastPCStore(llvm::BasicBlock &bb) -> llvm::StoreInst *;
auto getLastPc(llvm::BasicBlock *bb) -> unsigned;
auto getLastInstStartPc(llvm::BasicBlock *bb, bool allowEmpty = false, bool noRecovered = false) -> unsigned;

static inline auto isInstStart(llvm::Instruction *inst) -> bool { return inst->getMetadata("inststart"); }

static inline auto getLastPc(llvm::Function *f) -> unsigned { return getLastPc(&f->getEntryBlock()); }

template <typename T> static auto blocksOverlap(T *a, T *b) -> bool {
    unsigned aStart = getBlockAddress(a);
    unsigned aEnd = getLastPc(a);
    unsigned bStart = getBlockAddress(b);
    unsigned bEnd = getLastPc(b);
    return bStart > aStart && bEnd == aEnd;
}

template <typename T> static inline auto haveSameEndAddress(T *a, T *b) -> bool { return getLastPc(a) == getLastPc(b); }

template <typename T> static inline auto blockContains(T *bb, unsigned pc) -> bool {
    return pc >= getBlockAddress(bb) && pc <= getLastPc(bb);
}
} // namespace binrec

#endif
