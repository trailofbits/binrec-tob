#include "IR/Selectors.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include "SectionUtils.h"
#include <llvm/IR/Metadata.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <unordered_set>

using namespace binrec;
using namespace llvm;

namespace {
auto copyMetadata(Instruction *from, Instruction *to) -> bool {
    if (!from->hasMetadata())
        return false;

    SmallVector<std::pair<unsigned, MDNode *>, 8> mds;
    from->getAllMetadataOtherThanDebugLoc(mds);

    for (auto &it : mds) {
        unsigned kind = it.first;
        MDNode *md = it.second;
        to->setMetadata(kind, md);
    }

    return true;
}

auto patchSuccessors(BasicBlock *bb, std::map<Function *, BasicBlock *> &f2bb) -> bool {
    Instruction *term = bb->getTerminator();

    if (MDNode *md = term->getMetadata("succs")) {
        std::vector<Metadata *> operands;

        for (unsigned i = 0, l = md->getNumOperands(); i < l; i++) {
            auto *f = cast<Function>(cast<ValueAsMetadata>(md->getOperand(i))->getValue());
            assert(f2bb.find(f) != f2bb.end());
            BasicBlock *bb = f2bb[f];
            operands.push_back(ValueAsMetadata::get(BlockAddress::get(bb->getParent(), bb)));
            // operands.push_back(dyn_cast<MetadataAsValue>(BlockAddress::get(bb->getParent(),
            // bb))->getMetadata());
        }

        term->setMetadata("succs", MDNode::get(bb->getContext(), operands));

        return true;
    }

    return false;
}

auto getEntryF(Module &m) -> Function * {
    std::unordered_set<Function *> entryCandidates;
    // Fill candidates
    for (Function &f : LiftedFunctions{m}) {
        entryCandidates.insert(&f);
    }

    for (Function &f : LiftedFunctions{m}) {
        Instruction *term = f.getEntryBlock().getTerminator();
        if (MDNode *md = term->getMetadata("succs")) {
            for (unsigned i = 0, l = md->getNumOperands(); i < l; i++) {
                auto *f = cast<Function>(cast<ValueAsMetadata>(md->getOperand(i))->getValue());
                entryCandidates.erase(f); // if it is a succ's of another bb, cant be entry bb.
            }
        }
    }

    // If there is more than 1 BB or 0 BB to be entry, there is a problem.
    assert(entryCandidates.size() == 1 && "More than 1 BB can be entry, Investigate!");
    return *entryCandidates.begin();
}

void createEntryBB(Function *wrapper, std::map<Function *, BasicBlock *> &f2bb, Function &f) {
    LLVMContext &ctx = f.getContext();
    std::string fname(f.getName());
    std::string bbname("BB" + fname.substr(fname.rfind('_')));
    BasicBlock *dummy = BasicBlock::Create(ctx, bbname, wrapper);

    Type *argType = f.getArg(0)->getType();
    Value *arg = ConstantPointerNull::get(cast<PointerType>(argType));
    CallInst *call = CallInst::Create(&f, arg, "", dummy);
    ReturnInst::Create(ctx, dummy);

    InlineFunctionInfo info;
    InlineResult inlineResult = InlineFunction(*call, info);
    assert(inlineResult.isSuccess());

    copyMetadata(f.getEntryBlock().getTerminator(), dummy->getTerminator());

    f2bb[&f] = dummy;
}

/// S2E Merge all functions into one large function
class MergeFunctionsPass : public PassInfoMixin<MergeFunctionsPass> {
public:
    /// - create a new global wrapper function
    /// - for each existing function, create a dummy basic block in the wrapper:
    ///   BB_xxxxxx:
    ///     call @Func_xxxxxx()
    ///     ret void
    /// - inline all Func_xxxxxx functions
    /// - copy the metadata of each function to the dummy block, replacing function
    ///   pointers with the address of the replacement BB
    /// - remove the old function definitions
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        LLVMContext &ctx = m.getContext();
        auto *wrapper = cast<Function>(m.getOrInsertFunction("Func_wrapper", Type::getVoidTy(ctx)).getCallee());

        std::map<Function *, BasicBlock *> f2bb;
        Function *entryF = getEntryF(m);
        createEntryBB(wrapper, f2bb, *entryF);

        for (Function &f : LiftedFunctions{m}) {
            if (entryF == &f)
                continue;

            std::string fname(f.getName());
            std::string bbname("BB" + fname.substr(fname.rfind('_')));
            BasicBlock *dummy = BasicBlock::Create(ctx, bbname, wrapper);

            Type *argType = f.getArg(0)->getType();
            Value *arg = ConstantPointerNull::get(cast<PointerType>(argType));
            CallInst *call = CallInst::Create(&f, arg, "", dummy);
            ReturnInst::Create(ctx, dummy);

            InlineFunctionInfo info;
            InlineResult inlineResult = InlineFunction(*call, info);
            assert(inlineResult.isSuccess());

            copyMetadata(f.getEntryBlock().getTerminator(), dummy->getTerminator());

            f2bb[&f] = dummy;
        }

        for (auto &it : f2bb) {
            BasicBlock *bb = it.second;
            patchSuccessors(bb, f2bb);
        }

        for (auto &it : f2bb) {
            Function *f = it.first;
            f->eraseFromParent();
        }

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addMergeFunctionsPass(ModulePassManager &mpm) { mpm.addPass(MergeFunctionsPass()); }
