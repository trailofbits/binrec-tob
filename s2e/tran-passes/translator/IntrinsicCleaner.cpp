//===-- IntrinsicCleaner.cpp ----------------------------------------------===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "IntrinsicCleaner.h"

//#include "klee/Config/config.h"
//#include "llvm/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstrTypes.h"
//#include "llvm/Instruction.h"
//#include "llvm/Instructions.h"
#include "llvm/IR/IntrinsicInst.h"
#include "llvm/IR/LLVMContext.h"
//#include "llvm/Module.h"
#include "llvm/Pass.h"
//#include "llvm/IR/Type.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/CallSite.h"
#include "llvm/Transforms/Scalar.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
#include "llvm/IR/DataLayout.h"
#include "llvm/Support/ErrorHandling.h"
#include <llvm/Support/raw_ostream.h>

using namespace llvm;

char IntrinsicCleanerPass::ID;
static RegisterPass<IntrinsicCleanerPass> X("clean-intrinsics",
        "Clean Intrinsic functions", false, false);
/// ReplaceCallWith - This function is used when we want to lower an intrinsic
/// call to a call of an external function.  This handles hard cases such as
/// when there was already a prototype for the external function, and if that
/// prototype doesn't match the arguments we expect to pass in.
template <class ArgIt>
static CallInst *ReplaceCallWith(const char *NewFn, CallInst *CI,
                                 ArgIt ArgBegin, ArgIt ArgEnd,
                                 Type *RetTy)
{
    // If we haven't already looked up this function, check to see if the
    // program already contains a function with this name.
    Module *M = CI->getParent()->getParent()->getParent();
    // Get or insert the definition now.
    std::vector<Type *> ParamTys;
    for (ArgIt I = ArgBegin; I != ArgEnd; ++I)
        ParamTys.push_back((*I)->getType());

    llvm::ArrayRef<Type *> ParamTysA(&ParamTys[0], ParamTys.size());

    Constant* FCache = M->getOrInsertFunction(NewFn,
                                  FunctionType::get(RetTy, ParamTysA, false));

    IRBuilder<> Builder(CI);
    //IRBuilder<> Builder(CI->getParent(), CI);
    SmallVector<Value *, 8> Args(ArgBegin, ArgEnd);


    CallInst *NewCI = Builder.CreateCall(FCache, llvm::ArrayRef<Value *>(Args));
    NewCI->setName(CI->getName());
    if (!CI->use_empty())
        CI->replaceAllUsesWith(NewCI);
    CI->eraseFromParent();
    return NewCI;
}


void IntrinsicCleanerPass::replaceIntrinsicAdd(Module &M, CallInst *CI)
{
    Value *arg0 = CI->getArgOperand(0);
    Value *arg1 = CI->getArgOperand(1);

    IntegerType *itype = static_cast<IntegerType*>(arg0->getType());
    assert(itype);

    Function *f;
    switch(itype->getBitWidth()) {
        case 16: f = M.getFunction("uadds"); break;
        case 32: f = M.getFunction("uadd"); break;
        case 64: f = M.getFunction("uaddl"); break;
        default: assert(false && "Invalid intrinsic type");
    }

    assert(f && "Could not find intrinsic replacements for add with overflow");

    StructType *aggregate = static_cast<StructType*>(CI->getCalledFunction()->getReturnType());

    std::vector<Value*> args;

    Value *alloca = new AllocaInst(itype, NULL, "", CI);
    args.push_back(alloca);
    args.push_back(arg0);
    args.push_back(arg1);
    Value *overflow = CallInst::Create(f, args, "", CI);

    //Store the values in the aggregated type
    Value *aggrValPtr = new AllocaInst(aggregate, NULL, "", CI);
    Value *aggrVal = new LoadInst(aggrValPtr, "", CI);
    Value *addResult = new LoadInst(alloca, "", CI);
    InsertValueInst *insRes = InsertValueInst::Create(aggrVal, addResult, 0, "", CI);
    InsertValueInst *insOverflow = InsertValueInst::Create(insRes, overflow, 1, "", CI);
    CI->replaceAllUsesWith(insOverflow);
    CI->eraseFromParent();
}

/**
 * Inject a function of the following form:
 * define i1 @uadds(i16*, i16, i16) {
 *   %4 = add i16 %1, %2
 *   store i16 %4, i16* %0
 *   %5 = icmp ugt i16 %1, %2
 *   %6 = select i1 %5, i16 %1, i16 %2
 *   %7 = icmp ult i16 %4, %6
 *   ret i1 %7
 *  }
 *
 * These functions replace the add with overflow intrinsics
 * used by LLVM. These intrinsics have a {iXX, i1} return type,
 * which causes problems if the size of the type is less than 64 bits.
 * clang basically packs such a type into a 64-bits integer, which causes
 * a silent type mismatch and data corruptions when KLEE tries
 * to interpret such a value with its extract instructions.
 * We therefore manually implement the functions here, to avoid using clang.
 */

void IntrinsicCleanerPass::injectIntrinsicAddImplementation(Module &M, const std::string &name, unsigned bits)
{
    Function *f = M.getFunction(name);
    if (f) {
        assert(!f->isDeclaration());
        return;
    }

    LLVMContext &ctx = M.getContext();
    std::vector<Type*> argTypes;
    argTypes.push_back(Type::getIntNPtrTy(ctx, bits)); //Result
    argTypes.push_back(Type::getIntNTy(ctx, bits)); //a
    argTypes.push_back(Type::getIntNTy(ctx, bits)); //b

    FunctionType *type = FunctionType::get(Type::getInt1Ty(ctx), ArrayRef<Type*>(argTypes), false);
    f = dyn_cast<Function>(M.getOrInsertFunction(name, type));
    assert(f);

    BasicBlock *bb = BasicBlock::Create(ctx, "", f);
    IRBuilder<> builder(bb);

    //std::vector<Value*>args;
    //Function::arg_iterator it = f->arg_begin();
    //args.push_back(it++);
    //args.push_back(it++);
    //args.push_back(it++);
    //auto args = f->args();

    Value *res = builder.CreateAdd(f->getOperand(1), f->getOperand(2));
    builder.CreateStore(res, f->getOperand(0));
    Value *cmp = builder.CreateICmpUGT(f->getOperand(1), f->getOperand(2));
    Value *cond = builder.CreateSelect(cmp, f->getOperand(1), f->getOperand(2));
    Value *cmp1 = builder.CreateICmpULT(res, cond);
    builder.CreateRet(cmp1);
}

bool IntrinsicCleanerPass::runOnModule(Module &M)
{
    bool dirty = true;
    const llvm::DataLayout DataLayout(M.getDataLayout());
    IL = new llvm::IntrinsicLowering(DataLayout);
    injectIntrinsicAddImplementation(M, "uadds", 16);
    injectIntrinsicAddImplementation(M, "uadd", 32);
    injectIntrinsicAddImplementation(M, "uaddl", 64);

    for (Module::iterator f = M.begin(), fe = M.end(); f != fe; ++f)
        for (Function::iterator b = f->begin(), be = f->end(); b != be;)
            dirty |= runOnBasicBlock(*(b++), DataLayout);
    return dirty;
}


bool IntrinsicCleanerPass::runOnBasicBlock(BasicBlock &b, const llvm::DataLayout &DataLayout)
{
    bool dirty = false;

    unsigned WordSize = DataLayout.getPointerSizeInBits() / 8;
    for (BasicBlock::iterator i = b.begin(), ie = b.end(); i != ie;) {
        IntrinsicInst *ii = dyn_cast<IntrinsicInst>(&*i);
        // increment now since LowerIntrinsic deletion makes iterator invalid.
        ++i;
        if(ii) {
            CallSite CS(ii);

            switch (ii->getIntrinsicID()) {
                case Intrinsic::vastart:
                case Intrinsic::vaend:
                break;

                // Lower vacopy so that object resolution etc is handled by
                // normal instructions.
                //
                // FIXME: This is much more target dependent than just the word size,
                // however this works for x86-32 and x86-64.
                case Intrinsic::vacopy: { // (dst, src) -> *((i8**) dst) = *((i8**) src)
                    Value *dst = ii->getArgOperand(0);
                    Value *src = ii->getArgOperand(1);

                    if (WordSize == 4) {
                        Type *i8pp = PointerType::getUnqual(PointerType::getUnqual(Type::getInt8Ty(getGlobalContext())));
                        Value *castedDst = CastInst::CreatePointerCast(dst, i8pp, "vacopy.cast.dst", ii);
                        Value *castedSrc = CastInst::CreatePointerCast(src, i8pp, "vacopy.cast.src", ii);
                        Value *load = new LoadInst(castedSrc, "vacopy.read", ii);
                        new StoreInst(load, castedDst, false, ii);
                    } else {
                        assert(WordSize == 8 && "Invalid word size!");
                        Type *i64p = PointerType::getUnqual(Type::getInt64Ty(getGlobalContext()));
                        Value *pDst = CastInst::CreatePointerCast(dst, i64p, "vacopy.cast.dst", ii);
                        Value *pSrc = CastInst::CreatePointerCast(src, i64p, "vacopy.cast.src", ii);
                        Value *val = new LoadInst(pSrc, std::string(), ii); new StoreInst(val, pDst, ii);
                        Value *off = ConstantInt::get(Type::getInt64Ty(getGlobalContext()), 1);
                        pDst = GetElementPtrInst::Create(i64p,pDst, off, std::string(), ii);
                        pSrc = GetElementPtrInst::Create(i64p,pSrc, off, std::string(), ii);
                        val = new LoadInst(pSrc, std::string(), ii); new StoreInst(val, pDst, ii);
                        pDst = GetElementPtrInst::Create(i64p,pDst, off, std::string(), ii);
                        pSrc = GetElementPtrInst::Create(i64p,pSrc, off, std::string(), ii);
                        val = new LoadInst(pSrc, std::string(), ii); new StoreInst(val, pDst, ii);
                    }
                    ii->removeFromParent();
                    delete ii;
                    break;
                }

                case Intrinsic::dbg_value:
                case Intrinsic::dbg_declare:
                    // Remove these regardless of lower intrinsics flag. This can
                    // be removed once IntrinsicLowering is fixed to not have bad
                    // caches.
                    ii->eraseFromParent();
                    dirty = true;
                    break;
                    //case Intrinsic::memory_barrier:
                    //case Intrinsic::atomic_swap:
                    //case Intrinsic::atomic_cmp_swap:
                    //case Intrinsic::atomic_load_add:
                    //case Intrinsic::atomic_load_sub:
                    break;

                //case Intrinsic::powi:
                //    ReplaceFPIntrinsicWithCall(ii, "powif", "powi", "powil");
                //    dirty = true;
                //    break;

                case Intrinsic::uadd_with_overflow:
                    replaceIntrinsicAdd(*b.getParent()->getParent(), ii);
                    dirty = true;
                    break;

                case Intrinsic::trap:
                    // Link with abort
                    ReplaceCallWith("abort", ii, CS.arg_end(), CS.arg_end(),
                                Type::getVoidTy(getGlobalContext()));
                    dirty = true;
                    break;

                case Intrinsic::memset:
                case Intrinsic::memcpy:
                case Intrinsic::memmove: {
                    LLVMContext &Ctx = ii->getContext();

                    Value *dst = ii->getArgOperand(0);
                    Value *src = ii->getArgOperand(1);
                    Value *len = ii->getArgOperand(2);

                    BasicBlock *BB = ii->getParent();
                    Function *F = BB->getParent();

                    BasicBlock *exitBB = BB->splitBasicBlock(ii);
                    BasicBlock *headerBB = BasicBlock::Create(Ctx, Twine(), F, exitBB);
                    BasicBlock *bodyBB  = BasicBlock::Create(Ctx, Twine(), F, exitBB);

                    // Enter the loop header
                    BB->getTerminator()->eraseFromParent();
                    BranchInst::Create(headerBB, BB);

                    // Create loop index
                    PHINode *idx = PHINode::Create(len->getType(), 0, Twine(), headerBB);
                    idx->addIncoming(ConstantInt::get(len->getType(), 0), BB);

                    // Check loop condition, then move to the loop body or exit the loop
                    Value *loopCond = ICmpInst::Create(Instruction::ICmp, ICmpInst::ICMP_ULT,
                                                       idx, len, Twine(), headerBB);
                    BranchInst::Create(bodyBB, exitBB, loopCond, headerBB);

                    // Get value to store
                    Value *val;
                    if (ii->getIntrinsicID() == Intrinsic::memset) {
                        val = src;
                    } else {
                        Value *srcPtr = GetElementPtrInst::Create(src->getType(),src, idx, Twine(), bodyBB);
                        val = new LoadInst(srcPtr, Twine(), bodyBB);
                    }

                    // Store the value
                    Value* dstPtr = GetElementPtrInst::Create(dst->getType(),dst, idx, Twine(), bodyBB);
                    new StoreInst(val, dstPtr, bodyBB);

                    // Update index and branch back
                    Value* newIdx = BinaryOperator::Create(Instruction::Add,
                                                           idx, ConstantInt::get(len->getType(), 1), Twine(), bodyBB);
                    BranchInst::Create(headerBB, bodyBB);
                    idx->addIncoming(newIdx, bodyBB);

                    ii->eraseFromParent();

                    // Update iterators to continue in the next BB
                    i = exitBB->begin();
                    ie = exitBB->end();
                    break;
                }

                default:
                    if (LowerIntrinsics)
                        IL->LowerIntrinsicCall(ii);
                    dirty = true;
                    break;
            }
        }
    }

    return dirty;
}

