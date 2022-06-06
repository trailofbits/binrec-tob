#include "globalize_env.hpp"
#include "ir/selectors.hpp"
#include "pass_utils.hpp"
#include <algorithm>
#include <llvm/ADT/SetVector.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/IRBuilder.h>

using namespace llvm;

namespace binrec {


    static auto get_env_type_info(Module &m) -> MDNode *
    {
        DebugInfoFinder info;
        info.processModule(m);

        for (DIType *t : info.types()) {
            if (auto *ct = dyn_cast<DICompositeType>(t)) {
                if (ct->getName() == "CPUX86State")
                    return t;
            }
        }
        return nullptr;
    }

    // Sanity checking the index of a gep
    static auto get_index(GetElementPtrInst *gep) -> unsigned
    {
        failUnless(gep->getNumIndices() >= 2, "less than 2 indices");
        failUnless(cast<ConstantInt>(gep->getOperand(1))->isZero(), "non-zero first index");
        return cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
    }

    // Extract the field member names from the info-node (representing struct CPUX86State)
    static std::vector<StringRef> get_member_names(MDNode *info)
    {
        auto *ct = dyn_cast<DICompositeType>(info);
        failUnless(ct != nullptr, "Expected info to be of type DICompositeType");

        std::vector<StringRef> names;
        for (auto *e : ct->getElements()) {
            if (auto *ty = dyn_cast<DIType>(e)) {
                names.push_back(ty->getName());
            }
        }
        return names;
    }

    // Get a new (or return existing) GlobalVariable representing a field in the env_type struct
    static auto
    get_global(Module &m, unsigned index, StructType *env_type, std::vector<StringRef> const &names)
        -> GlobalVariable *
    {
        Type *ty = env_type->getElementType(index);
        return cast<GlobalVariable>(m.getOrInsertGlobal(names[index], ty));
    }

    static Instruction *
    replace_gep(GetElementPtrInst *gep, StructType *env_ty, std::vector<StringRef> const &names)
    {

        // Create (or get) a global variable representing the member in the CPUX86State struct.
        GlobalVariable *glob = get_global(*gep->getModule(), get_index(gep), env_ty, names);

        if (gep->getNumIndices() > 2) {
            // Replace the member access with access to the global variable
            // A GEP instruction for a CPUX86State field has at least 2 indexes:
            //
            //   Index #1 - Always 0, dereference the CPUV86State pointer
            //   Index #2 - The field index
            //
            // This pass will remove index #2, the field index, and replace it with a
            // 0 (index #1) to dereference the field pointer directly.
            //
            // For example, here is the index list to get the 4th item in the fpregs
            // array: (field #9)
            //
            //  i64 0, i32 9, i64 4, i32 0, i32 0
            //
            // This pass will replace this index list with:
            //
            //  i64 0, i64 4, i32 0, i32 0
            //
            // The first index is dereferencing fpregs and the second index is the
            // fpregs item.
            //
            // See GitHub issue #116 for more information:
            // https://github.com/trailofbits/binrec-prerelease/issues/116
            IRBuilder<> irb(gep);
            llvm::SmallVector<Value *, 5> indices;
            auto start = gep->idx_begin();

            // first index == 0 (dereference the field directly)
            indices.push_back(*start);
            // Move the index iterator up 2 to skip the field index
            std::advance(start, 2);

            std::copy(
                start,
                gep->idx_end(),
                std::back_inserter(indices)); // Assuming at least one index

            auto geprepl =
                irb.CreateInBoundsGEP(glob->getType()->getPointerElementType(), glob, indices);
            gep->replaceAllUsesWith(geprepl);
        } else {
            gep->replaceAllUsesWith(glob);
        }

        return gep;
    }

    // Replace GEPs with GEP to new global variables representing fields of the CPUX86State struct.
    // For other instructions, iterate users until a GEP is found.
    static void process_instruction(
        Instruction *inst,
        StructType *env_ty,
        std::vector<StringRef> const &names,
        SetVector<Instruction *> &toerase)
    {
        if (auto *gep = dyn_cast<GetElementPtrInst>(inst)) {
            toerase.insert(replace_gep(gep, env_ty, names));
        } else if (isa<LoadInst>(inst) || isa<PHINode>(inst) || isa<PtrToIntInst>(inst)) {
            for (auto use : inst->users())
                process_instruction(cast<Instruction>(use), env_ty, names, toerase);
        } else if (isa<CallInst>(inst)) {
            // Argument to a function call. No need to handle, explicitly.
            return;
        } else {
            inst->print(errs());
            failUnless(false, "Unexpected access to CPUX86State struct.");
        }
        // TODO (hbrodin): Consider if all instructions should be replaced? Below line...
        // toerase.insert(inst);
    }


    // NOLINTNEXTLINE
    auto GlobalizeEnvPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
    {
        // TODO: this appears to not be working for the env->fpregs field. custom-helpers.cpp
        // has a different address of fpregs than the helper functions do.
        GlobalVariable *env = m.getNamedGlobal("env");
        failUnless(env != nullptr, "Missing global env pointer");

        auto env_ty = cast<StructType>(
            cast<PointerType>(env->getType()->getPointerElementType())->getPointerElementType());

        auto helperbc = loadBitcodeFile(runlibDir() + "/op_helper.bc", m.getContext());
        failUnless(!!helperbc, "Failed to find the op_helper.bc-file");

        MDNode *env_info = get_env_type_info(*helperbc);
        failUnless(env_info, "no debug information found for env struct in op_helper.bc");

        auto names = get_member_names(env_info);

        SetVector<Instruction *> toerase;

        // Handle references to the global env var
        for (User *use : env->users())
            process_instruction(cast<Instruction>(use), env_ty, names, toerase);

        // Each of the lifted functions have env var as argument, replace all
        // usage in those functions with global variables.
        for (auto &f : LiftedFunctions{m}) {
            failUnless(f.arg_size() == 1, "Expected only a single argument");

            for (auto use : f.arg_begin()->users()) {
                process_instruction(cast<Instruction>(use), env_ty, names, toerase);
            }
        }
        // Drop all replaced instructions
        for (auto inst : toerase) {
            inst->eraseFromParent();
        }

        return PreservedAnalyses::allInSet<CFGAnalyses>();
    }


} // namespace binrec
