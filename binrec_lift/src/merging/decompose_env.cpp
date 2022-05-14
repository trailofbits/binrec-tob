#include "decompose_env.hpp"
#include "ir/selectors.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/DebugInfo.h>

using namespace binrec;
using namespace llvm;
using namespace std;

static auto get_env_type_info(unique_ptr<Module> m) -> MDNode *
{
    DebugInfoFinder info;
    info.processModule(*m);

    for (DIType *t : info.types()) {
        if (auto *ct = dyn_cast<DICompositeType>(t)) {
            if (ct->getName() == "CPUX86State")
                return t;
        }
    }

    return nullptr;
}

static auto get_index(GetElementPtrInst *gep) -> unsigned
{
    failUnless(gep->getNumIndices() >= 2, "less than 2 indices");
    failUnless(cast<ConstantInt>(gep->getOperand(1))->isZero(), "non-zero first index");
    return cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
}

static void get_member_names(MDNode *info, vector<StringRef> &names)
{
    auto *ct = dyn_cast<DICompositeType>(info);
    if (!ct) {
        return;
    }

    names.clear();
    for (auto *e : ct->getElements()) {
        if (auto *ty = dyn_cast<DIType>(e)) {
            names.push_back(ty->getName());
        }
    }
}

static auto get_global(Module &m, unsigned index, StructType *env_type, vector<StringRef> &names)
    -> GlobalVariable *
{
    Type *ty = env_type->getElementType(index);
    return cast<GlobalVariable>(m.getOrInsertGlobal(names[index], ty));
}

static void
process_inst(Module &m, Instruction *inst, StructType *env_type, vector<StringRef> &names)
{
    if (isa<LoadInst>(inst) || isa<PHINode>(inst)) {
        vector<User *> users{inst->user_begin(), inst->user_end()};
        for (User *use : users) {
            auto *i = cast<Instruction>(use);
            process_inst(m, i, env_type, names);
        }
    } else if (auto *gep = dyn_cast<GetElementPtrInst>(inst)) {
        GlobalVariable *glob = get_global(m, get_index(gep), env_type, names);
        if (gep->getNumIndices() > 2) {
            IRBuilder<> b(gep);
            vector<Value *> indices = {b.getInt32(0)};
            for (unsigned i = 2, i_upper_bound = gep->getNumIndices(); i < i_upper_bound; ++i)
                indices.push_back(gep->getOperand(i + 1));
            gep->replaceAllUsesWith(b.CreateInBoundsGEP(nullptr, glob, indices));
        } else {
            gep->replaceAllUsesWith(glob);
        }
    } else if (auto *bitcast = dyn_cast<BitCastInst>(inst)) {
        // The bitcast happens when LLVM generates a memcpy intrinsic which the struct is
        // passed in (e.g. helper_fldz_FT0).
        assert(bitcast->hasOneUse() && "The memcpy intrinsic uses the i8* only once.");
        auto *offset = cast<GetElementPtrInst>(bitcast->user_back());
        assert(offset->getNumIndices() == 1 && "Gep in memcpy is a single offset from a i8*.");
        auto *add_offset_base = cast<BinaryOperator>(offset->getOperand(1));
        assert(
            add_offset_base->getOpcode() == Instruction::Add &&
            "Memcpy offset is an add instruction.");
        uint64_t base = cast<ConstantInt>(add_offset_base->getOperand(1))->getZExtValue();
        unsigned element_index =
            m.getDataLayout().getStructLayout(env_type)->getElementContainingOffset(base);
        GlobalVariable *glob = get_global(m, element_index, env_type, names);
        bitcast->setOperand(0, glob);
        add_offset_base->replaceAllUsesWith(add_offset_base->getOperand(0));
    } else {
        llvm_unreachable("Unexpected access type to CPU state.");
    }
}

// NOLINTNEXTLINE
auto DecomposeEnvPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    GlobalVariable *env = m.getNamedGlobal("env");
    auto *env_type =
        cast<StructType>(cast<PointerType>(env->getType()->getPointerElementType())->getPointerElementType());

    auto helperbc = loadBitcodeFile(runlibDir() + "/op_helper.bc", m.getContext());
    MDNode *env_info = get_env_type_info(move(helperbc));
    failUnless(env_info, "no debug information found for env struct in op_helper.bc");

    vector<StringRef> names;
    get_member_names(env_info, names);

    for (User *use : env->users()) {
        if (auto ins = dyn_cast<Instruction>(use)) {
            auto f = ins->getFunction();
            if (is_lifted_function(*f)) {
                auto *load = cast<LoadInst>(use);
                process_inst(m, load, env_type, names);
            }
        }
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}