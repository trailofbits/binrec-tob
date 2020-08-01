#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

MDNode *getBlockMeta(BasicBlock *bb, StringRef kind) {
    assert(!bb->empty() && "BB is empty");
    return bb->getTerminator()->getMetadata(kind);
}

MDNode *getBlockMeta(Function *f, StringRef kind) {
    assert(!f->empty() && "function is empty");
    return getBlockMeta(&f->getEntryBlock(), kind);
}

void setBlockMeta(BasicBlock *bb, StringRef kind, MDNode *md) {
    assert(!bb->empty() && "BB is empty");
    bb->getTerminator()->setMetadata(kind, md);
}

void setBlockMeta(Function *f, StringRef kind, MDNode *md) {
    assert(!f->empty() && "function is empty");
    setBlockMeta(&f->getEntryBlock(), kind, md);
}

bool hasOperand(MDNode *md, Value *operand) {
    fori(i, 0, md->getNumOperands()) {
        if (dyn_cast<ValueAsMetadata>(md->getOperand(i))->getValue() == operand)
            return true;
    }
    return false;
}

//TODO(aaltinay): figure out if we need to change return type to MDTuple
MDNode *removeOperand(MDNode *md, unsigned index) {
    std::vector<Metadata*> operands;
    fori(i, 0, md->getNumOperands()) {
        if (i != index)
            operands.push_back(md->getOperand(i).get());
    }
    return MDNode::get(md->getContext(), operands);
}

MDNode *removeNullOperands(MDNode *md) {
    std::vector<Metadata*> operands;
    fori(i, 0, md->getNumOperands()) {
        if (md->getOperand(i).get() != NULL)
            operands.push_back(md->getOperand(i));
    }
    return operands.size() == 0 ? NULL : MDNode::get(md->getContext(), operands);
}

bool getBlockSuccs(BasicBlock *bb, std::vector<BasicBlock*> &succs) {
    MDNode *md = getBlockMeta(bb, "succs");
    if (!md)
        return false;

    succs.clear();
    fori(i, 0, md->getNumOperands()) {    
        BlockAddress *op = cast<BlockAddress>(dyn_cast<ValueAsMetadata>(md->getOperand(i))->getValue());
        // If we create op from the following line, BasicBlock that we get from getBasicBlock causes segfault.
        //BlockAddress *op = cast<BlockAddress>(MetadataAsValue::get(md->getContext(), md->getOperand(i).get()));
        BasicBlock *B = op->getBasicBlock();	
        succs.push_back(B);
    }

    return true;
}

void setBlockSuccs(BasicBlock *bb, const std::vector<BasicBlock*> &succs) {
    std::vector<Metadata*> operands;
    for (BasicBlock *succ : succs) {
        //operands.push_back(dyn_cast<MetadataAsValue>(BlockAddress::get(succ->getParent(), succ))->getMetadata());
        operands.push_back(ValueAsMetadata::get(BlockAddress::get(succ->getParent(), succ)));
    }
    setBlockMeta(bb, "succs", MDNode::get(bb->getContext(), operands));
}

bool getBlockSuccs(Function *f, std::vector<Function*> &succs) {
    MDNode *md = getBlockMeta(f, "succs");
    if (!md)
        return false;

    succs.clear();
    fori(i, 0, md->getNumOperands()){
        succs.push_back(cast<Function>(dyn_cast<ValueAsMetadata>(md->getOperand(i))->getValue()));
    }
    return true;
}

void setBlockSuccs(Function *f, const std::vector<Function*> &succs) {
    std::vector<Metadata*> succsMeta;
    for (Function *F : succs){
        succsMeta.push_back(ValueAsMetadata::get(F));
    }
    setBlockMeta(f, "succs", MDNode::get(f->getContext(), succsMeta));
}
