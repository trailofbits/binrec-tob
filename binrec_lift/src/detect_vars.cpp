#include "detect_vars.hpp"
#include "error.hpp"
#include "pass_utils.hpp"
#include "section_utils.hpp"

using namespace llvm;

char DetectVars::ID = 0;
static RegisterPass<DetectVars>
    X("detect-vars", "S2E Heuristically split section arrays into global variables", false, false);

static inline auto overlap(unsigned aindex, unsigned asize, unsigned bindex, unsigned bsize) -> bool
{
    return bindex + bsize > aindex && aindex + asize > bindex;
}

static auto getInitializer(GlobalVariable *global, unsigned index, IntegerType *elementType)
    -> Constant *
{
    Constant *ginit = global->getInitializer();
    uint64_t value = 0;

    if (!isa<ConstantAggregateZero>(ginit)) {
        auto *arr = cast<ConstantDataArray>(ginit);
        assert(arr->getElementByteSize() == 1);

        // asuming little-endian
        for (unsigned i = 0, iUpperBound = elementType->getBitWidth() / 8; i < iUpperBound; ++i)
            value += arr->getElementAsInteger(index + i) << i;
    }

    return ConstantInt::get(elementType, value);
}

static auto detectSectionVars(Module &m, section_meta_t &s) -> bool
{
    bool changed = false;

    // XXX: the sizes map should be placed in metadata to be used in later
    // executions of the pass for slightly more safety checks
    std::map<unsigned, unsigned> sizes;

    for (User *use : s.global->users()) {
        if (auto *gep = dyn_cast<GEPOperator>(use)) {
            assert(cast<ConstantInt>(gep->getOperand(1))->isZero());
            auto *indexConst = dyn_cast<ConstantInt>(gep->getOperand(2));

            if (!indexConst)
                continue;

            unsigned index = indexConst->getZExtValue();

            assert(gep->hasOneUse());
            auto *expr = cast<ConstantExpr>(*gep->use_begin());
            assert(expr->getOpcode() == Instruction::BitCast);
            auto *targetType = cast<PointerType>(expr->getType());
            auto *elementType = cast<IntegerType>(targetType->getElementType());
            assert(elementType->getBitWidth() % 8 == 0);
            unsigned size = elementType->getBitWidth() / 8;

            for (auto it : sizes) {
                if (overlap(it.first, it.second, index, size)) {
                    LLVM_ERROR(error)
                        << "overlapping globals in " << s.global->getName() << ": (" << it.first
                        << ", " << it.second << ") and (" << index << ", " << size << ")";
                    throw std::runtime_error{error};
                }
            }

            sizes[index] = size;

            Constant *initializer = getInitializer(s.global, index, elementType);
            Twine name = s.global->getName() + "_" + Twine(index);
            auto *g = new GlobalVariable(
                m,
                elementType,
                false,
                GlobalValue::InternalLinkage,
                initializer,
                name);

            DBG("replace " << size << " bytes at index " << index << " of @" << s.global->getName()
                           << " with @" << g->getName());

            expr->replaceAllUsesWith(g);
            changed = true;
        }
    }

    return changed;
}

auto DetectVars::runOnModule(Module &m) -> bool
{
    struct param {
        Module &m;
        bool changed;
    } p = {m, false};

    mapToSections(
        m,
        [](section_meta_t &s, void *param) {
            auto *p = (struct param *)param;
            p->changed |= detectSectionVars(p->m, s);
            return false;
        },
        &p);

    if (p.changed)
        WARNING("replaced section getelementptr accesses with new globals");

    return p.changed;
}
