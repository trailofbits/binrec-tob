#include "llvm/IR/DebugInfo.h"

#include "UnflattenEnv.h"
#include "PassUtils.h"

using namespace llvm;

char UnflattenEnv::ID = 0;
static RegisterPass<UnflattenEnv> x("unflatten-env",
        "S2E Convert pointer offsets to env struct member accesses", false, false);

bool UnflattenEnv::doInitialization(Module &m) {
    env = m.getNamedGlobal("env");
    envType = cast<StructType>(cast<PointerType>(
                env->getType()->getElementType())->getElementType());
    layout = new DataLayout(&m);
    envLayout = layout->getStructLayout(envType);

    return true;
}

bool UnflattenEnv::doFinalization(Module &m) {
    delete layout;
    return false;
}

unsigned UnflattenEnv::getMemberIndex(CompositeType *ty, uint64_t offset) {
    if (StructType *structTy = dyn_cast<StructType>(ty))
        return layout->getStructLayout(structTy)->getElementContainingOffset(offset);

    SequentialType *seqTy = cast<SequentialType>(ty);
    return offset / layout->getTypeAllocSize(seqTy->getElementType());
}

bool UnflattenEnv::runOnFunction(Function &f) {
    if (!isRecoveredBlock(&f))
        return false;

    Argument *envArg = &*(f.arg_begin());
    foreach2(iuse, envArg->user_begin(), envArg->user_end()) {
        LoadInst *load;

        // get constant offset added to env pointer
        if (GetElementPtrInst *gep = dyn_cast<GetElementPtrInst>(*iuse)) {
            failUnless(gep->getNumIndices() == 1, "multiple indices");
            failUnless(gep->hasAllZeroIndices(), "non-zero index");
            failUnless(gep->hasOneUse(), "multiple GEP uses");
            load = cast<LoadInst>(*gep->user_begin());
        } else {
            load = cast<LoadInst>(*iuse);
        }
        
        foreach2(juse, load->user_begin(), load->user_end()) {
            Instruction *loadUse = cast<Instruction>(*juse);
            
            uint64_t offset;
            IntToPtrInst *ptrCast = dyn_cast<IntToPtrInst>(loadUse);

            if (ptrCast) {
                offset = 0;
            } else if (loadUse->isBinaryOp() && loadUse->getOpcode() == Instruction::Add) {
                assert(loadUse->hasOneUse() && "multiple add uses");
                offset = cast<ConstantInt>(loadUse->getOperand(1))->getZExtValue();
                ptrCast = cast<IntToPtrInst>(*loadUse->user_begin());
            } else {
                errs() << "load use is no add or inttoptr:" << *loadUse << "\n";
                assert(false);
            }

            // get type that is loaded/stored
            Type *targetTy = cast<PointerType>(ptrCast->getType())->getElementType();

            // find env struct member to which the offset belongs
            unsigned member = envLayout->getElementContainingOffset(offset);

            // replace use with struct member access
            IRBuilder<> b(ptrCast);
            std::vector<Value*> indices = {b.getInt32(0), b.getInt32(member)};

            // add indices for composite member types
            Type *memberTy = envType->getElementType(member);
            offset -= envLayout->getElementOffset(member);

            while (memberTy != targetTy && isa<CompositeType>(memberTy) &&
                    !isa<PointerType>(memberTy)) {
                CompositeType *ty = cast<CompositeType>(memberTy);
                member = getMemberIndex(ty, offset);
                indices.push_back(b.getInt32(member));

                Value *idx[] = {b.getInt32(0), b.getInt32(member)};
                offset -= layout->getIndexedOffset(memberTy->getPointerTo(), idx);

                memberTy = ty->getTypeAtIndex(member);
            }

            failUnless(offset == 0, "non-zero offset remaining");

            Value *repl = b.CreateInBoundsGEP(b.CreateLoad(env), indices);

            if (repl->getType() != ptrCast->getType())
                repl = b.CreatePointerCast(repl, ptrCast->getType());

            ptrCast->replaceAllUsesWith(repl);
        }
    }

    return true;
}
