#include "llvm/IR/DebugInfo.h"

#include "DecomposeEnv.h"
#include "PassUtils.h"

using namespace llvm;

char DecomposeEnv::ID = 0;
static RegisterPass<DecomposeEnv> x("decompose-env",
        "S2E Replace CPU struct members with global variables", false, false);

static MDNode *getEnvTypeInfo(std::unique_ptr<llvm::Module> m) {
    DebugInfoFinder info;
    info.processModule(*m);
    
    for (DIType *T : info.types()) {
        if (auto *CT = dyn_cast<DICompositeType>(T)) {
            if (CT->getName() == "CPUX86State")
                return T;
        }
    }
/*           if (auto *S = CT->getRawIdentifier())
              O << " (identifier: '" << S->getString() << "')";
        }
    foreach2(itype, info.types().begin(), info.types().end()) {
        DIDescriptor desc(*itype);
        if (desc.isCompositeType()) {
            DICompositeType ty(desc);
            if (ty.getName() == "CPUX86State")
                return *itype;
        }
    }
*/
    return NULL;
}

void DecomposeEnv::getMemberNames(MDNode *info) {
    auto *CT = dyn_cast<DICompositeType>(info);
    if (!CT) {
        return;
    }
   // DICompositeType ty(info);
   // DIArray types = CT->getTypeArray();

    names.clear();
    // This works as expected after porting.
    for (auto *E : CT->getElements()) {
        if (auto *ty = dyn_cast<DIType>(E)){
            names.push_back(ty->getName());
        }
    }
/*
    fori(i, 0, types.getNumElements()) {
        DIDescriptor tydesc(types.getElement(i));
        if (tydesc.isType()) {
            DIType ty(tydesc);
            names.push_back(ty.getName());
        }
    }*/
}

static unsigned getIndex(GetElementPtrInst *gep) {
    failUnless(gep->getNumIndices() >= 2, "less than 2 indices");
    failUnless(cast<ConstantInt>(gep->getOperand(1))->isZero(), "non-zero first index");
    return cast<ConstantInt>(gep->getOperand(2))->getZExtValue();
}

GlobalVariable *DecomposeEnv::getGlobal(Module &m, unsigned index) {
    Type *ty = envType->getElementType(index);
    return cast<GlobalVariable>(m.getOrInsertGlobal(names[index], ty));
}

void DecomposeEnv::processInst(Module &m, Instruction *inst) {
    if (isa<LoadInst>(inst) || isa<PHINode>(inst)) {
        foreach_use_as(inst, Instruction, use, processInst(m, use));
    }
    else ifcast(GetElementPtrInst, gep, inst) {
        
        GlobalVariable *glob = getGlobal(m, getIndex(gep));

        if (gep->getNumIndices() > 2) {
            assert(isa<CompositeType>(glob->getType()->getElementType()));
            IRBuilder<> b(gep);
            std::vector<Value*> indices = {b.getInt32(0)};
            fori(i, 2, gep->getNumIndices())
                indices.push_back(gep->getOperand(i + 1));
            gep->replaceAllUsesWith(b.CreateInBoundsGEP(glob, indices));
        } else {
            gep->replaceAllUsesWith(glob);
        }
    }
    else {
        assert(!"euhm...");
    }
}

bool DecomposeEnv::runOnModule(Module &m) {
    GlobalVariable *env = m.getNamedGlobal("env");
    envType = cast<StructType>(cast<PointerType>(
                env->getType()->getElementType())->getElementType());

    auto helperbc = loadBitcodeFile(runlibDir() + "/op_helper.bc");
    MDNode *envInfo = getEnvTypeInfo(std::move(helperbc));
    failUnless(envInfo, "no debug information found for env struct in op_helper.bc");

    getMemberNames(envInfo);

    foreach_use_as(env, LoadInst, load, processInst(m, load));

    return true;
}
