#include <set>
#include "EnvToAllocas.h"
#include "AddMemArray.h"
#include "PassUtils.h"

using namespace llvm;

char EnvToAllocas::ID = 0;
static RegisterPass<EnvToAllocas> x("env-to-allocas",
        "S2E Transform environment globals into allocas in @wrapper",
        false, false);

static bool isEnvVar(GlobalVariable *glob) {
    // FIXME: tag these in metadata in env decomposition pass?
    static std::set<std::string> envVarNames = {
        "PC", "cc_src", "cc_dst", "cc_op", "R_EAX", "R_EBX", "R_ECX", "R_EDX",
        "R_ESI", "R_EDI", "R_EBP", "R_ESP", "fpus", "fpuc", "fpstt", "fpregs", 
        "fptags", "fp_status", "ft0", "df", "mflags", "debug_verbosity", "stack"
    };
    return envVarNames.find(glob->getName().str()) != envVarNames.end();
}

static bool canBeMoved(GlobalVariable *glob, Function *wrapper) {
    if (!glob->hasName() || glob->getName() == MEMNAME)
        return false;

    foreach_use(glob, use, {
        ifcast(ConstantExpr, expr, *use) {
            DBG("no alloca for " << glob->getName() <<
                ": used by ConstantExpr: " << *expr);
            return false;
        }
        else ifcast(Instruction, instr, *use) {
            const Function *f = instr->getParent()->getParent();
            if (f != wrapper) {
                DBG("no alloca for " << glob->getName() <<
                    ": used outside @wrapper in @" << f->getName());
                return false;
            }
        } else {
            errs() << "Unexpected use of global: " << **use << "\n";
            exit(1);
        }
    });

    return true;
}

bool EnvToAllocas::runOnModule(Module &m) {
    // Move each global that is only used within @wrapper
    Function *wrapper = m.getFunction("wrapper");
    //assert(wrapper && "wrapper not found");
    if(!wrapper) {
        wrapper = m.getFunction("main");
    }
    assert(wrapper && "main not found");

    IRBuilder<> b(&*(wrapper->getEntryBlock().begin()));
    std::vector<GlobalVariable*> eraseList;

    foreach2(glob, m.global_begin(), m.global_end()) {
        if (!isEnvVar(&*glob) || !canBeMoved(&*glob, wrapper))
            continue;

        // Transform GEP operators in constant expressions into instructions,
        // since pointers to local variables are not constant
        std::vector<Value*> idx;
        std::vector<std::pair<Instruction*, Value*>> replaceList;

        foreach_use_if(glob, GEPOperator, gep, {
            IRBuilder<> b(m.getContext());
            replaceList.clear();

            foreach_use_as(gep, Instruction, parent, {
                b.SetInsertPoint(parent);
                idx.clear();
                fori(i, 0, gep->getNumIndices())
                    idx.push_back(gep->getOperand(i + 1));
                Value *newGep = b.CreateInBoundsGEP(gep->getPointerOperand(), idx);
                replaceList.push_back(std::make_pair(parent, newGep));
            });

            for (const auto &it : replaceList)
                it.first->replaceUsesOfWith(gep, it.second);
        });

        DBG("transforming " << glob->getName() << " into alloca");

        AllocaInst *alloca = b.CreateAlloca(glob->getType()->getElementType(),
                NULL, glob->getName());

        if (glob->hasInitializer())
            b.CreateStore(glob->getInitializer(), alloca);

        glob->replaceAllUsesWith(alloca);
        eraseList.push_back(&*glob);
    }

    for (GlobalVariable *glob : eraseList)
        glob->eraseFromParent();

    return true;
}

