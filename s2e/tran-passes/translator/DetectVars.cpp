#include "DetectVars.h"
#include "PassUtils.h"
#include "SectionUtils.h"

using namespace llvm;

char DetectVars::ID = 0;
static RegisterPass<DetectVars> X("detect-vars",
        "S2E Heuristically split section arrays into global variables",
        false, false);

static inline bool overlap(unsigned aindex, unsigned asize,
        unsigned bindex, unsigned bsize) {
    return bindex + bsize > aindex && aindex + asize > bindex;
}

static Constant *getInitializer(GlobalVariable *global,
        unsigned index, IntegerType *elementType) {
    Constant *ginit = global->getInitializer();
    uint64_t value = 0;

    if (!isa<ConstantAggregateZero>(ginit)) {
        ConstantDataArray *arr = cast<ConstantDataArray>(ginit);
        assert(arr->getElementByteSize() == 1);

        // asuming little-endian
        fori(i, 0, elementType->getBitWidth() / 8)
            value += arr->getElementAsInteger(index + i) << i;
    }

    return ConstantInt::get(elementType, value);
}

static bool detectSectionVars(Module &m, section_meta_t &s) {
    bool changed = false;

    // XXX: the sizes map should be placed in metadata to be used in later
    // executions of the pass for slightly more safety checks
    std::map<unsigned, unsigned> sizes;

    foreach_use_if(s.global, GEPOperator, gep, {
        assert(cast<ConstantInt>(gep->getOperand(1))->isZero());
        ConstantInt *indexConst = dyn_cast<ConstantInt>(gep->getOperand(2));

        if (!indexConst)
            continue;

        unsigned index = indexConst->getZExtValue();

        assert(gep->hasOneUse());
        ConstantExpr *expr = cast<ConstantExpr>(*gep->use_begin());
        assert(expr->getOpcode() == Instruction::BitCast);
        PointerType *targetType = cast<PointerType>(expr->getType());
        IntegerType *elementType = cast<IntegerType>(targetType->getElementType());
        assert(elementType->getBitWidth() % 8 == 0);
        unsigned size = elementType->getBitWidth() / 8;

        for (auto it : sizes) {
            if (overlap(it.first, it.second, index, size)) {
                errs() << "overlapping globals in " << s.global->getName() <<
                    ": (" << it.first << ", " << it.second << ") and (" <<
                    index << ", " << size << ")\n";
                exit(1);
            }
        }

        sizes[index] = size;

        Constant *initializer = getInitializer(s.global, index, elementType);
        Twine name = s.global->getName() + "_" + Twine(index);
        GlobalVariable *g = new GlobalVariable(m, elementType, false,
                GlobalValue::InternalLinkage, initializer, name);

        DBG("replace " << size << " bytes at index " << index << " of @" <<
                s.global->getName() << " with @" << g->getName());

        expr->replaceAllUsesWith(g);
        changed = true;
    });

    return changed;
}

bool DetectVars::runOnModule(Module &m) {
    struct param {
        Module &m;
        bool changed;
    } p = {m, false};

    mapToSections(m, [](section_meta_t &s, void *param) {
        struct param *p = (struct param*)param;
        p->changed |= detectSectionVars(p->m, s);
        return false;
    }, &p);

    if (p.changed)
        WARNING("replaced section getelementptr accesses with new globals");

    return p.changed;
}
