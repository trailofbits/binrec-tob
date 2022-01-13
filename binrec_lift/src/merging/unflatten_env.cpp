#include "unflatten_env.hpp"
#include "ir/selectors.hpp"
#include "pass_utils.hpp"
#include <memory>

using namespace binrec;
using namespace llvm;
using namespace std;

static auto get_member_index(Type *ty, uint64_t offset, DataLayout *layout) -> unsigned
{
    if (auto *struct_ty = dyn_cast<StructType>(ty))
        return layout->getStructLayout(struct_ty)->getElementContainingOffset(offset);

    auto *seq_ty = cast<ArrayType>(ty);
    return offset / layout->getTypeAllocSize(seq_ty->getElementType());
}

// NOLINTNEXTLINE
auto UnflattenEnvPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    GlobalVariable *env = m.getNamedGlobal("env");
    auto *env_type =
        cast<StructType>(cast<PointerType>(env->getType()->getElementType())->getElementType());
    unique_ptr<DataLayout> layout = make_unique<DataLayout>(&m);
    const StructLayout *env_layout = layout->getStructLayout(env_type);

    for (auto &f : LiftedFunctions{m}) {
        Argument *env_arg = &*(f.arg_begin());
        for (User *iuse : env_arg->users()) {
            LoadInst *load;

            // get constant offset added to env pointer
            if (auto *gep = dyn_cast<GetElementPtrInst>(iuse)) {
                if (gep->getNumIndices() != 1)
                    continue; // Ignore

                if (!gep->hasAllZeroIndices())
                    continue; // Ignore

                if (!gep->hasOneUse())
                    continue; // Ignore
                /*
                failUnless(gep->getNumIndices() == 1, "multiple indices");
                failUnless(gep->hasAllZeroIndices(), "non-zero index");
                failUnless(gep->hasOneUse(), "multiple GEP uses");
                */
                load = cast<LoadInst>(*gep->user_begin());
            } /*else if (auto *ptrtoint = dyn_cast<PtrToIntInst>(iuse)) {
                errs() << "Ignore ptrtoint "; 
                ptrtoint->print(errs());
                continue;
            } */
            else if (auto ld = dyn_cast<LoadInst>(iuse)) {
                //load = cast<LoadInst>(iuse);
                load = ld;
            } else {
                outs() << "Ignore use of env: ";
                iuse->print(outs());
                outs() << "\n";
                continue;

            }

            for (User *juse : load->users()) {
                auto *load_use = cast<Instruction>(juse);

                uint64_t offset;
                auto *ptr_cast = dyn_cast<IntToPtrInst>(load_use);

                if (ptr_cast) {
                    offset = 0;
                } else if (load_use->isBinaryOp() && load_use->getOpcode() == Instruction::Add) {
                    assert(load_use->hasOneUse() && "multiple add uses");
                    offset = cast<ConstantInt>(load_use->getOperand(1))->getZExtValue();
                    ptr_cast = cast<IntToPtrInst>(*load_use->user_begin());
                } else {
                    errs() << "load use is no add or inttoptr:" << *load_use << "\n";
                    assert(false);
                }

                // get type that is loaded/stored
                Type *target_ty = cast<PointerType>(ptr_cast->getType())->getElementType();

                // find env struct member to which the offset belongs
                unsigned member = env_layout->getElementContainingOffset(offset);

                // replace use with struct member access
                IRBuilder<> b(ptr_cast);
                vector<Value *> indices = {b.getInt32(0), b.getInt32(member)};

                // add indices for composite member types
                Type *member_ty = env_type->getElementType(member);
                offset -= env_layout->getElementOffset(member);

                while (member_ty != target_ty &&
                       (member_ty->isArrayTy() || member_ty->isStructTy()) &&
                       !member_ty->isPointerTy())
                {
                    member = get_member_index(member_ty, offset, layout.get());
                    indices.push_back(b.getInt32(member));

                    array<Value *, 2> idx = {b.getInt32(0), b.getInt32(member)};
                    offset -= layout->getIndexedOffsetInType(member_ty, idx);

                    if (auto *arr_ty = dyn_cast<ArrayType>(member_ty)) {
                        member_ty = arr_ty->getElementType();
                    } else if (auto *struc_ty = dyn_cast<StructType>(member_ty)) {
                        member_ty = struc_ty->getTypeAtIndex(member);
                    } else {
                        assert(false);
                    }
                }

                failUnless(offset == 0, "non-zero offset remaining");

                Value *repl = b.CreateInBoundsGEP(b.CreateLoad(env), indices);

                if (repl->getType() != ptr_cast->getType())
                    repl = b.CreatePointerCast(repl, ptr_cast->getType());

                ptr_cast->replaceAllUsesWith(repl);
            }
        }
    }

    return PreservedAnalyses::none();
}
