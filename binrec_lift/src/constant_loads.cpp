#include "constant_loads.hpp"
#include "error.hpp"
#include "pass_utils.hpp"
#include "section_utils.hpp"

using namespace llvm;

#define PASS_NAME "constant_loads"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

struct param_t {
    Module &module;
    uint32_t address;
    MDNode *sectionMeta;
};

struct repl_t {
    User *gep;
    uint32_t address;
    Value *offset;
    GlobalVariable *sectionGlobal;
    uint32_t sectionBase;
};

char ConstantLoads::ID = 0;
static RegisterPass<ConstantLoads>
    X("constant-loads",
      "S2E Replace loads of constant addresses from copied sections with "
      "array offsets",
      false,
      false);

static auto checkSection(section_meta_t &s, void *param) -> bool
{
    param_t &p = *(param_t *)param;

    if (p.address < s.loadBase || p.address >= s.loadBase + s.size)
        return false;

    // if (!s.global->isConstant())
    //    return false;

    p.sectionMeta = s.md;
    return true;
}

static void pushReplacement(std::list<repl_t> &replacements, User *gep, Value *offset, param_t &p)
{
    repl_t r{gep, p.address, offset, nullptr, 0};
    section_meta_t s;
    readSectionMeta(s, p.sectionMeta);
    r.sectionGlobal = s.global;
    r.sectionBase = s.loadBase;
    replacements.push_back(r);
}

auto ConstantLoads::runOnModule(Module &m) -> bool
{
    GlobalVariable *memory = m.getNamedGlobal("memory");
    PASS_ASSERT(memory && "no global memory array");

    std::list<repl_t> replacements;
    param_t p{m, 0, nullptr};
    IntegerType *i32Ty = IntegerType::getInt32Ty(m.getContext());
    ConstantInt *zero = ConstantInt::get(i32Ty, 0);

    // For each memory GEP operand, check if the offset computation
    // involves a large constant in a known read-only data section
    for (User *use : memory->users()) {
        User *gep = cast<User>(use);

        PASS_ASSERT(isa<GetElementPtrInst>(gep) || isa<GEPOperator>(gep));
        PASS_ASSERT(gep->getNumOperands() == 2);

        Value *offset = gep->getOperand(1);
        if (auto *constInt = dyn_cast<ConstantInt>(offset)) {
            p.address = constInt->getZExtValue();
            if (mapToSections(p.module, checkSection, &p))
                pushReplacement(replacements, gep, zero, p);
        } else if (auto *inst = dyn_cast<Instruction>(offset)) {
            if (inst->isBinaryOp() && inst->getOpcode() == Instruction::Add) {
                for (int i = 0; i < 2; i++) {
                    if (auto *constInt = dyn_cast<ConstantInt>(inst->getOperand(i))) {
                        p.address = constInt->getZExtValue();
                        if (mapToSections(p.module, checkSection, &p)) {
                            pushReplacement(replacements, gep, inst->getOperand(1 - i), p);
                            break;
                        }
                    }
                }
            }
        }
    }

    if (replacements.size() == 0)
        return false;

    for (const repl_t &r : replacements) {
        DBG("const gep:" << *r.gep);
        Value *newGep = nullptr;

        if (auto *gep = dyn_cast<GetElementPtrInst>(r.gep)) {
            IRBuilder<> b(gep);

            Value *sectionOffset = b.CreateAdd(b.getInt32(r.address - r.sectionBase), r.offset);
            Value *idx[] = {zero, sectionOffset};
            newGep = b.CreateInBoundsGEP(r.sectionGlobal, idx, gep->getName());

            gep->replaceAllUsesWith(newGep);
            gep->eraseFromParent();
        } else {
            auto *op = cast<GEPOperator>(r.gep);
            PASS_ASSERT(r.offset == zero && "GEPOperator used with non-constant offset");
            Constant *idx[] = {zero, ConstantInt::get(i32Ty, r.address - r.sectionBase)};
            newGep = ConstantExpr::getInBoundsGetElementPtr(nullptr, r.sectionGlobal, idx);
            op->replaceAllUsesWith(newGep);
        }

        DBG("  replace with:" << *newGep);
    }

    return true;
    ;
}
