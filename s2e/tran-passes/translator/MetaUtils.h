#ifndef _METAUTILS_H
#define _METAUTILS_H

using namespace llvm;

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>

MDNode *getBlockMeta(BasicBlock *bb, StringRef kind);
MDNode *getBlockMeta(Function *f, StringRef kind);
void setBlockMeta(BasicBlock *bb, StringRef kind, MDNode *md);
void setBlockMeta(Function *f, StringRef kind, MDNode *md);

template<typename block_t>
static void setBlockMeta(block_t *bb, StringRef kind, uint32_t i) {
    LLVMContext &ctx = bb->getContext();
    setBlockMeta(bb, kind, MDNode::get(ctx,
                ConstantAsMetadata::get(ConstantInt::get(Type::getInt32Ty(ctx), i))));
}

bool hasOperand(MDNode *md, Value *operand);
MDNode *removeOperand(MDNode *md, unsigned index);
MDNode *removeNullOperands(MDNode *md);

bool getBlockSuccs(BasicBlock *bb, std::vector<BasicBlock*> &succs);
void setBlockSuccs(BasicBlock *bb, const std::vector<BasicBlock*> &succs);
bool getBlockSuccs(Function *f, std::vector<Function*> &succs);
void setBlockSuccs(Function *f, const std::vector<Function*> &succs);

#endif  // _METAUTILS_H
