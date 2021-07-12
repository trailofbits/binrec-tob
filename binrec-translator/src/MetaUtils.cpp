#include "MetaUtils.h"
#include "PassUtils.h"
#include <llvm/IR/Metadata.h>

using namespace llvm;

auto binrec::getBlockMeta(const BasicBlock *bb, StringRef kind) -> MDNode * {
    assert(!bb->empty() && "BB is empty");
    return bb->getTerminator()->getMetadata(kind);
}

auto binrec::getBlockMeta(const Function *f, StringRef kind) -> MDNode * {
    assert(!f->empty() && "function is empty");
    return getBlockMeta(&f->getEntryBlock(), kind);
}

void binrec::setBlockMeta(BasicBlock *bb, StringRef kind, MDNode *md) {
    assert(!bb->empty() && "BB is empty");
    bb->getTerminator()->setMetadata(kind, md);
}

void binrec::setBlockMeta(Function *f, StringRef kind, MDNode *md) {
    assert(!f->empty() && "function is empty");
    setBlockMeta(&f->getEntryBlock(), kind, md);
}

auto binrec::removeNullOperands(MDNode *md) -> MDNode * {
    std::vector<Metadata *> operands;
    for (unsigned i = 0, iUpperBound = md->getNumOperands(); i < iUpperBound; ++i) {
        if (md->getOperand(i).get() != nullptr)
            operands.push_back(md->getOperand(i));
    }
    return operands.empty() ? nullptr : MDNode::get(md->getContext(), operands);
}

auto binrec::getBlockSuccs(BasicBlock *bb, std::vector<BasicBlock *> &succs) -> bool {
    MDNode *md = getBlockMeta(bb, "succs");
    if (!md)
        return false;

    succs.clear();
    for (unsigned i = 0, iUpperBound = md->getNumOperands(); i < iUpperBound; ++i) {
        Value *successor = cast<ValueAsMetadata>(md->getOperand(i))->getValue();
        if (isa<ConstantExpr>(successor)) {
            continue;
        }
        BasicBlock *succBb = isa<Function>(successor) ? &cast<Function>(successor)->getEntryBlock()
                                                      : cast<BlockAddress>(successor)->getBasicBlock();
        succs.push_back(succBb);
    }

    return true;
}

void binrec::setBlockSuccs(BasicBlock *bb, const std::vector<BasicBlock *> &succs) {
    std::vector<Metadata *> operands;
    for (BasicBlock *succ : succs) {
        Function *successorFunction = succ->getParent();
        Value *successorVal = &successorFunction->getEntryBlock() == succ ? cast<Value>(successorFunction)
                                                                          : BlockAddress::get(successorFunction, succ);
        operands.push_back(ValueAsMetadata::get(successorVal));
    }
    MDNode *md = operands.empty() ? nullptr : MDNode::get(bb->getContext(), operands);
    setBlockMeta(bb, "succs", md);
}

auto binrec::getBlockSuccs(Function *f, std::vector<Function *> &succs) -> bool {
    MDNode *md = getBlockMeta(f, "succs");
    if (!md)
        return false;

    succs.clear();
    for (unsigned i = 0, iUpperBound = md->getNumOperands(); i < iUpperBound; ++i) {
        succs.push_back(cast<Function>(dyn_cast<ValueAsMetadata>(md->getOperand(i))->getValue()));
    }
    return true;
}

void binrec::setBlockSuccs(Function *f, const std::vector<Function *> &succs) {
    std::vector<Metadata *> succsMeta;
    succsMeta.reserve(succs.size());
    for (Function *succ : succs) {
        succsMeta.push_back(ValueAsMetadata::get(succ));
    }
    setBlockMeta(f, "succs", MDNode::get(f->getContext(), succsMeta));
}
