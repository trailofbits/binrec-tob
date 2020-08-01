#include "MergeFunctions.h"
#include "PassUtils.h"
#include "SectionUtils.h"
#include "llvm/IR/Metadata.h"
#include <unordered_set>

using namespace llvm;

char MergeFunctionsPass::ID = 0;
static RegisterPass<MergeFunctionsPass> X("merge-functions",
        "S2E Merge all functions into one large function", false, false);

static bool copyMetadata(Instruction *from, Instruction *to) {
    if (!from->hasMetadata())
        return false;

    SmallVector<std::pair<unsigned, MDNode*>, 8> mds;
    from->getAllMetadataOtherThanDebugLoc(mds);

    foreach(it, mds) {
        unsigned kind = it->first;
        MDNode *md = it->second;
        to->setMetadata(kind, md);
    }

    return true;
}

static bool patchSuccessors(BasicBlock *bb, std::map<Function*, BasicBlock*> &f2bb) {
    TerminatorInst *term = bb->getTerminator();

    if (MDNode *md = term->getMetadata("succs")) {
        std::vector<Metadata*> operands;

        for (unsigned i = 0, l = md->getNumOperands(); i < l; i++) {
            Function *f = cast<Function>(cast<ValueAsMetadata>(md->getOperand(i))->getValue());
            assert(f2bb.find(f) != f2bb.end());
            BasicBlock *bb = f2bb[f];
            operands.push_back(ValueAsMetadata::get(BlockAddress::get(bb->getParent(), bb)));
            //operands.push_back(dyn_cast<MetadataAsValue>(BlockAddress::get(bb->getParent(), bb))->getMetadata());
        }

        term->setMetadata("succs", MDNode::get(bb->getContext(), operands));

        return true;
    }

    return false;
}


static Function* getEntryF(Module &m) {
    std::unordered_set<Function*> entryCandidates;
    //Fill candidates
    for(Function &f : m) {
        if (!f.getName().startswith("Func_"))
            continue;
        entryCandidates.insert(&f);
    }

    for(Function &f : m) {
        if (!f.getName().startswith("Func_"))
            continue;

        TerminatorInst *term = f.getEntryBlock().getTerminator();
        if (MDNode *md = term->getMetadata("succs")) {
            for (unsigned i = 0, l = md->getNumOperands(); i < l; i++) {
                Function *f = cast<Function>(cast<ValueAsMetadata>(md->getOperand(i))->getValue());
                entryCandidates.erase(f); //if it is a succ's of another bb, cant be entry bb.
            }
        }
    }

    //If there is more than 1 BB or 0 BB to be entry, there is a problem.
    assert(entryCandidates.size() == 1 && "More than 1 BB can be entry, Investigate!");
    return *entryCandidates.begin();
}
static void createEntryBB(Function *wrapper, std::map<Function*, BasicBlock*> &f2bb, Function &f) {
    LLVMContext &ctx = f.getContext();
    std::string fname(f.getName());
    std::string bbname("BB" + fname.substr(fname.rfind('_')));
    BasicBlock *dummy = BasicBlock::Create(ctx, bbname, wrapper);

    Type *argType = f.getArgumentList().front().getType();
    Value *arg = ConstantPointerNull::get(cast<PointerType>(argType));
    CallInst *call = CallInst::Create(&f, arg, "", dummy);
    ReturnInst::Create(ctx, dummy);

    InlineFunctionInfo info;
    bool success = InlineFunction(call, info);
    assert(success);

    copyMetadata(f.getEntryBlock().getTerminator(), dummy->getTerminator());

    f2bb[&f] = dummy;
}

static void fixEntryBB(Function *wrapper, std::map<Function*, BasicBlock*> &f2bb) {
    std::unordered_set<Function*> entryCandidates;
    //Fill candidates
    for(auto &p : f2bb){
        entryCandidates.insert(p.first);
    }


    for(auto &p : f2bb){
        TerminatorInst *term = p.second->getTerminator();

        if (MDNode *md = term->getMetadata("succs")) {
            for (unsigned i = 0, l = md->getNumOperands(); i < l; i++) {
                Function *f = cast<Function>(cast<ValueAsMetadata>(md->getOperand(i))->getValue());
                entryCandidates.erase(f); //if it is a succ's of another bb, cant be entry bb.
            }
        }
    }

    //If there is more than 1 BB or 0 BB to be entry, there is a problem.
    assert(entryCandidates.size() == 1 && "More than 1 BB can be entry, Investigate!");
    BasicBlock *entry = f2bb[*entryCandidates.begin()];
    BasicBlock &curEntry = wrapper->getEntryBlock();
    if(entry != &curEntry)
        entry->moveBefore(&curEntry);
}
/*
 * - create a new global wrapper function
 * - for each existing function, create a dummy basic block in the wrapper:
 *   BB_xxxxxx:
 *     call @Func_xxxxxx()
 *     ret void
 * - inline all Func_xxxxxx functions
 * - copy the metadata of each function to the dummy block, replacing function
 *   pointers with the address of the replacement BB
 * - remove the old function definitions
 */
bool MergeFunctionsPass::runOnModule(Module &m) {
    LLVMContext &ctx = m.getContext();
    Function *wrapper = cast<Function>(m.getOrInsertFunction(
                "wrapper", Type::getVoidTy(ctx), NULL));

    std::map<Function*, BasicBlock*> f2bb;
    Function *entryF = getEntryF(m);
    createEntryBB(wrapper, f2bb, *entryF);

    for(Function &f : m) {
        if (!f.getName().startswith("Func_") || entryF == &f)
            continue;

        std::string fname(f.getName());
        std::string bbname("BB" + fname.substr(fname.rfind('_')));
        BasicBlock *dummy = BasicBlock::Create(ctx, bbname, wrapper);

        Type *argType = f.getArgumentList().front().getType();
        Value *arg = ConstantPointerNull::get(cast<PointerType>(argType));
        CallInst *call = CallInst::Create(&f, arg, "", dummy);
        ReturnInst::Create(ctx, dummy);

        InlineFunctionInfo info;
        bool success = InlineFunction(call, info);
        assert(success);

        copyMetadata(f.getEntryBlock().getTerminator(), dummy->getTerminator());

        f2bb[&f] = dummy;
    }

    //fixEntryBB(wrapper, f2bb);
    foreach(it, f2bb) {
        BasicBlock *bb = it->second;
        patchSuccessors(bb, f2bb);
    }

    foreach(it, f2bb) {
        Function *f = it->first;
        f->eraseFromParent();
    }

    return true;
}
