#ifndef BINREC_METAUTILS_H
#define BINREC_METAUTILS_H

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <vector>

namespace binrec {
auto getBlockMeta(const llvm::BasicBlock *bb, llvm::StringRef kind) -> llvm::MDNode *;
auto getBlockMeta(const llvm::Function *f, llvm::StringRef kind) -> llvm::MDNode *;
void setBlockMeta(llvm::BasicBlock *bb, llvm::StringRef kind, llvm::MDNode *md);
void setBlockMeta(llvm::Function *f, llvm::StringRef kind, llvm::MDNode *md);

template <typename block_t> static void setBlockMeta(block_t *bb, llvm::StringRef kind, uint32_t i) {
    llvm::LLVMContext &ctx = bb->getContext();
    setBlockMeta(
        bb, kind,
        llvm::MDNode::get(ctx, llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(llvm::Type::getInt32Ty(ctx), i))));
}

auto removeNullOperands(llvm::MDNode *md) -> llvm::MDNode *;

auto getBlockSuccs(llvm::BasicBlock *bb, std::vector<llvm::BasicBlock *> &succs) -> bool;
void setBlockSuccs(llvm::BasicBlock *bb, const std::vector<llvm::BasicBlock *> &succs);
auto getBlockSuccs(llvm::Function *f, std::vector<llvm::Function *> &succs) -> bool;
void setBlockSuccs(llvm::Function *f, const std::vector<llvm::Function *> &succs);
} // namespace binrec

#endif