#include "error.hpp"
#include "env_to_allocas.hpp"
#include "pass_utils.hpp"
#include <set>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
    auto isEnvVar(GlobalVariable *glob) -> bool
    {
        // FIXME: tag these in metadata in env decomposition pass?
        static set<string> envVarNames = {
            "PC",    "cc_src", "cc_dst", "cc_op",           "R_EAX",  "R_EBX",
            "R_ECX", "R_EDX",  "R_ESI",  "R_EDI",           "R_EBP",  "R_ESP",
            "fpus",  "fpuc",   "fpstt",  "fpregs",          "fptags", "fp_status",
            "ft0",   "df",     "mflags", "debug_verbosity", "stack"};
        return envVarNames.find(glob->getName().str()) != envVarNames.end();
    }

    auto canBeMoved(GlobalVariable *glob, Function *wrapper) -> bool
    {
        if (!glob->hasName())
            return false;

        for (Use &use : glob->uses()) {
            if (auto *expr = dyn_cast<ConstantExpr>(use)) {
                DBG("no alloca for " << glob->getName() << ": used by ConstantExpr: " << *expr);
                return false;
            } else if (auto *instr = dyn_cast<Instruction>(use)) {
                const Function *f = instr->getParent()->getParent();
                if (f != wrapper) {
                    DBG("no alloca for " << glob->getName() << ": used outside @wrapper in @"
                                         << f->getName());
                    return false;
                }
            } else {
                LLVM_ERROR(error) << "Unexpected use of global: " << *use;
                throw std::runtime_error{error};
            }
        }

        return true;
    }
} // namespace

// NOLINTNEXTLINE
auto EnvToAllocasPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    // Move each global that is only used within @wrapper
    Function *wrapper = m.getFunction("Func_wrapper");
    // assert(wrapper && "Func_wrapper not found");
    if (!wrapper) {
        wrapper = m.getFunction("main");
    }
    assert(wrapper && "main not found");

    IRBuilder<> b(&*(wrapper->getEntryBlock().begin()));
    vector<GlobalVariable *> eraseList;

    for (GlobalVariable &glob : m.globals()) {
        if (!isEnvVar(&glob) || !canBeMoved(&glob, wrapper)) {
            continue;
        }

        // Transform GEP operators in constant expressions into instructions,
        // since pointers to local variables are not constant
        vector<Value *> idx;
        vector<pair<Instruction *, Value *>> replaceList;

        for (auto &use : glob.uses()) {
            auto gep = dyn_cast<GEPOperator>(use);
            if (gep == nullptr) {
                continue;
            }

            IRBuilder<> b(m.getContext());
            replaceList.clear();

            for (auto &use : gep->uses()) {
                auto parent = cast<Instruction>(use);
                b.SetInsertPoint(parent);
                idx.clear();
                for (unsigned int i = 0; i < gep->getNumIndices(); ++i) {
                    idx.push_back(gep->getOperand(i + 1));
                }
                Value *newGep = b.CreateInBoundsGEP(gep->getPointerOperand(), idx);
                replaceList.emplace_back(parent, newGep);
            }

            for (const auto &it : replaceList) {
                it.first->replaceUsesOfWith(gep, it.second);
            }
        }

        DBG("transforming " << glob.getName() << " into alloca");

        AllocaInst *alloca =
            b.CreateAlloca(glob.getType()->getElementType(), nullptr, glob.getName());

        if (glob.hasInitializer()) {
            b.CreateStore(glob.getInitializer(), alloca);
        }

        glob.replaceAllUsesWith(alloca);
        eraseList.push_back(&glob);
    }

    for (GlobalVariable *glob : eraseList) {
        glob->eraseFromParent();
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
