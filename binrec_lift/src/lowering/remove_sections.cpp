#include "remove_sections.hpp"
#include "error.hpp"
#include "pass_utils.hpp"
#include "section_utils.hpp"

#define PASS_NAME "remove_sections"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
    auto getSectionOffset(GlobalVariable *g, GlobalVariable *s, unsigned &offset) -> bool
    {
        if (!g->hasName())
            return false;

        string prefix = s->getName().str() + "_";

        if (!g->getName().startswith(prefix))
            return false;

        offset = stoul(g->getName().substr(prefix.size()).str());
        return true;
    }

    auto getInt32(Module &m, unsigned i) -> ConstantInt *
    {
        return ConstantInt::get(Type::getInt32Ty(m.getContext()), i);
    }

    auto removeSectionReferences(section_meta_t &s, void *) -> bool
    {
        Module &m = *s.global->getParent();

        // First replace created variables with array offsets again so that they
        for (GlobalVariable &g : m.globals()) {
            unsigned offset = 0;
            if (getSectionOffset(&g, s.global, offset)) {
                WARNING(
                    "generated @" << g.getName()
                                  << " still around at "
                                     "-remove-sections, converting back to getelementptr");
                array<Constant *, 2> idx = {getInt32(m, 0), getInt32(m, offset)};
                Constant *gep = ConstantExpr::getGetElementPtr(nullptr, s.global, idx);
                g.replaceAllUsesWith(ConstantExpr::getBitCast(gep, g.getType()));
            }
        }

        // Collect uses, split in instructions and operators
        list<GetElementPtrInst *> insts;
        list<GEPOperator *> ops;

        for (User *use : s.global->users()) {
            User *gep = cast<User>(use);
            if (isa<GetElementPtrInst>(gep)) {
                insts.push_back(cast<GetElementPtrInst>(gep));
            } else if (isa<GEPOperator>(gep)) {
                ops.push_back(cast<GEPOperator>(gep));
            } else {
                PASS_ASSERT(!"expected GEP as use of section global");
            }
            PASS_ASSERT(gep->getNumOperands() == 3);
            PASS_ASSERT(cast<ConstantInt>(gep->getOperand(1))->getZExtValue() == 0);
        }

        // Replace uses with getelementptrs into @memory
        IRBuilder<> irb(m.getContext());

        for (GetElementPtrInst *gep : insts) {
            Value *offset = gep->getOperand(2);
            irb.SetInsertPoint(gep);
            Value *idx = irb.CreateAdd(irb.getInt32(s.loadBase), offset, "offset");
            Value *newPtr = irb.CreateIntToPtr(idx, gep->getType());
            gep->replaceAllUsesWith(newPtr);
            gep->eraseFromParent();
        }

        for (GEPOperator *gep : ops) {
            unsigned offset = cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
            Constant *idx = irb.getInt32(s.loadBase + offset);
            Value *newPtr = irb.CreateIntToPtr(idx, gep->getType());
            gep->replaceAllUsesWith(newPtr);
        }

        // Remove global definition
        s.global->eraseFromParent();

        return false;
    }
} // namespace

// NOLINTNEXTLINE
auto RemoveSectionsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    if (NamedMDNode *md = m.getNamedMetadata(SECTIONS_METADATA)) {
        mapToSections(m, removeSectionReferences, nullptr);
        md->eraseFromParent();
        return PreservedAnalyses::allInSet<CFGAnalyses>();
    }
    return PreservedAnalyses::none();
}
