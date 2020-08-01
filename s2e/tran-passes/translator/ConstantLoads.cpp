#include "ConstantLoads.h"
#include "PassUtils.h"
#include "SectionUtils.h"

using namespace llvm;

typedef struct {
    Module &module;
    uint32_t address;
    MDNode *sectionMeta;
} param_t;

typedef struct {
    User *gep;
    uint32_t address;
    Value *offset;
    GlobalVariable *sectionGlobal;
    uint32_t sectionBase;
} repl_t;

char ConstantLoads::ID = 0;
static RegisterPass<ConstantLoads> X("constant-loads",
        "S2E Replace loads of constant addresses from copied sections with "
        "array offsets", false, false);

static bool checkSection(section_meta_t &s, void *param) {
    param_t &p = *(param_t*)param;

    if (p.address < s.loadBase || p.address >= s.loadBase + s.size)
        return false;

    //if (!s.global->isConstant())
    //    return false;

    p.sectionMeta = s.md;
    return true;
}

static void pushReplacement(std::list<repl_t> &replacements,
        User *gep, Value *offset, param_t &p) {
    repl_t r = {
        .gep = gep,
        .address = p.address,
        .offset = offset
    };
    section_meta_t s;
    readSectionMeta(s, p.sectionMeta);
    r.sectionGlobal = s.global;
    r.sectionBase = s.loadBase;
    replacements.push_back(r);
}

bool ConstantLoads::runOnModule(Module &m) {
    GlobalVariable *memory = m.getNamedGlobal("memory");
    assert(memory && "no global memory array");

    std::list<repl_t> replacements;
    param_t p = {.module = m};
    IntegerType *i32Ty = IntegerType::getInt32Ty(m.getContext());
    ConstantInt *zero = ConstantInt::get(i32Ty, 0);

    // For each memory GEP operand, check if the offset computation
    // involves a large constant in a known read-only data section
    foreach_use_as(memory, User, gep, {
        assert(isa<GetElementPtrInst>(gep) || isa<GEPOperator>(gep));
        assert(gep->getNumOperands() == 2);
        Value *offset = gep->getOperand(1);

        if (ConstantInt *constInt = dyn_cast<ConstantInt>(offset)) {
            p.address = constInt->getZExtValue();
            if (mapToSections(p.module, checkSection, &p))
                pushReplacement(replacements, gep, zero, p);
        }
        else if (Instruction *inst = dyn_cast<Instruction>(offset)) {
            if (inst->isBinaryOp() && inst->getOpcode() == Instruction::Add) {
                for (int i = 0; i < 2; i++) {
                    if (ConstantInt *constInt = dyn_cast<ConstantInt>(inst->getOperand(i))) {
                        p.address = constInt->getZExtValue();
                        if (mapToSections(p.module, checkSection, &p)) {
                            pushReplacement(replacements, gep, inst->getOperand(1 - i), p);
                            break;
                        }
                    }
                }
            }
        }
    });

    if (replacements.size() == 0)
        return false;

    for (const repl_t &r : replacements) {
        DBG("const gep:" << *r.gep);
        Value *newGep;

        if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(r.gep)) {
            IRBuilder<> b(gep);

            Value *sectionOffset = b.CreateAdd(b.getInt32(r.address - r.sectionBase), r.offset);
            Value *idx[] = {zero, sectionOffset};
            newGep = b.CreateInBoundsGEP(r.sectionGlobal, idx, gep->getName());

            gep->replaceAllUsesWith(newGep);
            gep->eraseFromParent();
        } else {
            GEPOperator *op = cast<GEPOperator>(r.gep);
            assert(r.offset == zero && "GEPOperator used with non-constant offset");
            Constant *idx[] = {zero, ConstantInt::get(i32Ty, r.address - r.sectionBase)};
            newGep = ConstantExpr::getInBoundsGetElementPtr(nullptr, r.sectionGlobal, idx);
            op->replaceAllUsesWith(newGep);
        }

        DBG("  replace with:" << *newGep);
    }

    return true;;
}
