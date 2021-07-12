#include "PassUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/DebugInfo.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>

using namespace llvm;

using std::unique_ptr;
using std::vector;

namespace {
auto getEnvTypeInfo(unique_ptr<Module> m) -> MDNode * {
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

auto getIndex(GetElementPtrInst *gep) -> unsigned {
    failUnless(gep->getNumIndices() >= 2, "less than 2 indices");
    failUnless(cast<ConstantInt>(gep->getOperand(1))->isZero(), "non-zero first index");
    return cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
}

/// Replace CPU state struct members with global variables
class DecomposeEnvPass : public PassInfoMixin<DecomposeEnvPass> {
    vector<StringRef> names;
    StructType *envType{};

public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        GlobalVariable *env = m.getNamedGlobal("env");
        envType = cast<StructType>(cast<PointerType>(env->getType()->getElementType())->getElementType());

        auto helperbc = loadBitcodeFile(runlibDir() + "/op_helper.bc", m.getContext());
        MDNode *envInfo = getEnvTypeInfo(move(helperbc));
        failUnless(envInfo, "no debug information found for env struct in op_helper.bc");

        getMemberNames(envInfo);

        for (User *use : env->users()) {
            auto *load = cast<LoadInst>(use);
            processInst(m, load);
        }

        return PreservedAnalyses::allInSet<CFGAnalyses>();
    }

private:
    void getMemberNames(MDNode *info) {
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

    auto getGlobal(Module &m, unsigned index) -> GlobalVariable * {
        Type *ty = envType->getElementType(index);
        return cast<GlobalVariable>(m.getOrInsertGlobal(names[index], ty));
    }

    void processInst(Module &m, Instruction *inst) {
        if (isa<LoadInst>(inst) || isa<PHINode>(inst)) {
            vector<User *> users{inst->user_begin(), inst->user_end()};
            for (User *use : users) {
                auto *i = cast<Instruction>(use);
                processInst(m, i);
            }
        } else if (auto *gep = dyn_cast<GetElementPtrInst>(inst)) {
            GlobalVariable *glob = getGlobal(m, getIndex(gep));
            if (gep->getNumIndices() > 2) {
                IRBuilder<> b(gep);
                vector<Value *> indices = {b.getInt32(0)};
                for (unsigned i = 2, iUpperBound = gep->getNumIndices(); i < iUpperBound; ++i)
                    indices.push_back(gep->getOperand(i + 1));
                gep->replaceAllUsesWith(b.CreateInBoundsGEP(glob, indices));
            } else {
                gep->replaceAllUsesWith(glob);
            }
        } else if (auto *bitcast = dyn_cast<BitCastInst>(inst)) {
            // The bitcast happens when LLVM generates a memcpy intrinsic which the struct is passed in (e.g.
            // helper_fldz_FT0).
            assert(bitcast->hasOneUse() && "The memcpy intrinsic uses the i8* only once.");
            auto *offset = cast<GetElementPtrInst>(bitcast->user_back());
            assert(offset->getNumIndices() == 1 && "Gep in memcpy is a single offset from a i8*.");
            auto *addOffsetBase = cast<BinaryOperator>(offset->getOperand(1));
            assert(addOffsetBase->getOpcode() == Instruction::Add && "Memcpy offset is an add instruction.");
            uint64_t base = cast<ConstantInt>(addOffsetBase->getOperand(1))->getZExtValue();
            unsigned elementIndex = m.getDataLayout().getStructLayout(envType)->getElementContainingOffset(base);
            GlobalVariable *glob = getGlobal(m, elementIndex);
            bitcast->setOperand(0, glob);
            addOffsetBase->replaceAllUsesWith(addOffsetBase->getOperand(0));
        } else {
            llvm_unreachable("Unexpected access type to CPU state.");
        }
    }
};
} // namespace

void binrec::addDecomposeEnvPass(ModulePassManager &mpm) { mpm.addPass(DecomposeEnvPass()); }
