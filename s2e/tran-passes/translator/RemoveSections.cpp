#include "RemoveSections.h"
#include "PassUtils.h"
#include "SectionUtils.h"
#include "AddMemArray.h"

using namespace llvm;

char RemoveSections::ID = 0;
static RegisterPass<RemoveSections> X("remove-sections",
        "S2E Remove section globals and replace references to them with "
        "constant getelementptr instructions", false, false);

static bool getSectionOffset(GlobalVariable *g, GlobalVariable *s, unsigned &offset) {
    if (!g->hasName())
        return false;

    std::string prefix = s->getName().str() + "_";

    if (!g->getName().startswith(prefix))
        return false;

    offset = std::stoul(g->getName().substr(prefix.size()));
    return true;
}

static inline ConstantInt *getInt32(Module &m, unsigned i) {
    return ConstantInt::get(Type::getInt32Ty(m.getContext()), i);
}

static bool removeSectionReferences(section_meta_t &s, void *param) {
    Module &m = *s.global->getParent();

    // First replace created variables with array offsets again so that they
    foreach2(g, m.global_begin(), m.global_end()) {
        unsigned offset;
        if (getSectionOffset(&*g, s.global, offset)) {
            WARNING("generated @" << g->getName() << " still around at "
                    "-remove-sections, converting back to getelementptr");
            Constant *idx[] = {getInt32(m, 0), getInt32(m, offset)};
            Constant *gep = ConstantExpr::getGetElementPtr(nullptr, s.global, idx);
            g->replaceAllUsesWith(ConstantExpr::getBitCast(gep, g->getType()));
        }
    }

    // Collect uses, split in instructions and operators
    std::list<GetElementPtrInst*> insts;
    std::list<GEPOperator*> ops;

    foreach_use_as(s.global, User, gep, {
        if (isa<GetElementPtrInst>(gep)) {
            insts.push_back(cast<GetElementPtrInst>(gep));
        } else if (isa<GEPOperator>(gep)) {
            ops.push_back(cast<GEPOperator>(gep));
        } else {
            gep->dump();
            assert(!"expected GEP as use of section global");
        }

        assert(gep->getNumOperands() == 3);
        assert(cast<ConstantInt>(gep->getOperand(1))->getZExtValue() == 0);
    });

    // Replace uses with getelementptrs into @memory
    GlobalVariable *memory = m.getNamedGlobal(MEMNAME);
    IRBuilder<> b(m.getContext());

    for (GetElementPtrInst *gep : insts) {
        Value *offset = gep->getOperand(2);
        b.SetInsertPoint(gep);
        Value *idx = b.CreateAdd(b.getInt32(s.loadBase), offset, "offset");
        Value *newGep = b.CreateInBoundsGEP(memory, idx);
        gep->replaceAllUsesWith(newGep);
        gep->eraseFromParent();
    }

    for (GEPOperator *gep : ops) {
        unsigned offset = cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
        Constant *idx = b.getInt32(s.loadBase + offset);
        Constant *newGep = ConstantExpr::getInBoundsGetElementPtr(nullptr, memory, idx);
        gep->replaceAllUsesWith(newGep);
    }

    // Remove global definition
    s.global->eraseFromParent();

    return false;
}

bool RemoveSections::runOnModule(Module &m) {
    if (NamedMDNode *md = m.getNamedMetadata(SECTIONS_METADATA)) {
        mapToSections(m, removeSectionReferences, NULL);
        md->eraseFromParent();
        return true;
    }
    return false;

}
