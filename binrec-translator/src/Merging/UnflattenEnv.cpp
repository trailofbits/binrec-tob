#include "IR/Selectors.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/PassManager.h>
#include <memory>

using namespace binrec;
using namespace llvm;

namespace {
/// S2E Convert pointer offsets to env struct member accesses
class UnflattenEnvPass : public PassInfoMixin<UnflattenEnvPass> {
    GlobalVariable *env{};
    StructType *envType{};
    std::unique_ptr<DataLayout> layout;
    const StructLayout *envLayout{};

public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        doInitialization(m);

        for (auto &f : LiftedFunctions{m}) {
            Argument *envArg = &*(f.arg_begin());
            for (User *iuse : envArg->users()) {
                LoadInst *load = nullptr;

                // get constant offset added to env pointer
                if (auto *gep = dyn_cast<GetElementPtrInst>(iuse)) {
                    failUnless(gep->getNumIndices() == 1, "multiple indices");
                    failUnless(gep->hasAllZeroIndices(), "non-zero index");
                    failUnless(gep->hasOneUse(), "multiple GEP uses");
                    load = cast<LoadInst>(*gep->user_begin());
                } else {
                    load = cast<LoadInst>(iuse);
                }

                for (User *juse : load->users()) {
                    auto *loadUse = cast<Instruction>(juse);

                    uint64_t offset = 0;
                    auto *ptrCast = dyn_cast<IntToPtrInst>(loadUse);

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
                    std::vector<Value *> indices = {b.getInt32(0), b.getInt32(member)};

                    // add indices for composite member types
                    Type *memberTy = envType->getElementType(member);
                    offset -= envLayout->getElementOffset(member);

                    while (memberTy != targetTy && (memberTy->isArrayTy() || memberTy->isStructTy()) &&
                           !memberTy->isPointerTy()) {
                        member = getMemberIndex(memberTy, offset);
                        indices.push_back(b.getInt32(member));

                        std::array<Value *, 2> idx = {b.getInt32(0), b.getInt32(member)};
                        offset -= layout->getIndexedOffsetInType(memberTy, idx);

                        if (auto *arrTy = dyn_cast<ArrayType>(memberTy)) {
                            memberTy = arrTy->getElementType();
                        } else if (auto *strucTy = dyn_cast<StructType>(memberTy)) {
                            memberTy = strucTy->getTypeAtIndex(member);
                        } else {
                            assert(false);
                        }
                    }

                    failUnless(offset == 0, "non-zero offset remaining");

                    Value *repl = b.CreateInBoundsGEP(b.CreateLoad(env), indices);

                    if (repl->getType() != ptrCast->getType())
                        repl = b.CreatePointerCast(repl, ptrCast->getType());

                    ptrCast->replaceAllUsesWith(repl);
                }
            }
        }

        doFinalization(m);

        return PreservedAnalyses::none();
    }

private:
    void doInitialization(Module &m) {
        env = m.getNamedGlobal("env");
        envType = cast<StructType>(cast<PointerType>(env->getType()->getElementType())->getElementType());
        layout = std::make_unique<DataLayout>(&m);
        envLayout = layout->getStructLayout(envType);
    }

    void doFinalization(Module &m) { layout = nullptr; }

    auto getMemberIndex(Type *ty, uint64_t offset) -> unsigned {
        if (auto *structTy = dyn_cast<StructType>(ty))
            return layout->getStructLayout(structTy)->getElementContainingOffset(offset);

        auto *seqTy = cast<ArrayType>(ty);
        return offset / layout->getTypeAllocSize(seqTy->getElementType());
    }
};
} // namespace

void binrec::addUnflattenEnvPass(llvm::ModulePassManager &mpm) { mpm.addPass(UnflattenEnvPass()); }
