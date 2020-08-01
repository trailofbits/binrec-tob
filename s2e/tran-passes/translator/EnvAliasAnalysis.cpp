#include "llvm/InitializePasses.h"
#include "llvm/PassAnalysisSupport.h"
#include "llvm/PassSupport.h"
#include <llvm/Analysis/ValueTracking.h>
#include "EnvAliasAnalysis.h"
#include "PassUtils.h"


using namespace llvm;

static const Value *getRegister(const Value *ptr) {
    if (const GlobalVariable *g = dyn_cast<GlobalVariable>(ptr)) {
        if (g->hasName() && g->getName().startswith("R_"))
            return g;
        if (g->hasName() && g->getName().startswith(".bss_"))  // FIXME: should remove this?
            return g;
    }
    else if (const AllocaInst *g = dyn_cast<AllocaInst>(ptr)) {
        if (g->hasName() && g->getName().startswith("R_")) {
            return g;
        }
    }
    return NULL;
}

AliasResult EnvAAResult::alias(const MemoryLocation &LocA, const MemoryLocation &LocB) {
    const Value *A = GetUnderlyingObject(LocA.Ptr, DL);
    const Value *B = GetUnderlyingObject(LocB.Ptr, DL);

    if (const Value *reg = getRegister(A)) {
        if (reg != getRegister(B))
            return NoAlias;
    } else {
        return NoAlias;
    }

    // TODO: only registers are handled now, should also incorporate rest of
    // CPU env struct

    return AAResultBase::alias(LocA, LocB);
}

ModRefInfo EnvAAResult::getModRefInfo(ImmutableCallSite CS,
                                          const MemoryLocation &Loc) {
    const Function *f = CS.getCalledFunction();
    const Value *g = getRegister(GetUnderlyingObject(Loc.Ptr, DL));

    if (f && f->hasName() && g) {
        if (f->getName().startswith("__stub") ||
                f->getName() == "helper_stub_trampoline") {
            return MRI_NoModRef;
        }
    }
    return AAResultBase::getModRefInfo(CS, Loc);
}

EnvAAResult EnvAA::run(Function &F, AnalysisManager<Function> *AM) {
  return EnvAAResult(F.getParent()->getDataLayout(),
                         AM->getResult<TargetLibraryAnalysis>(F));
}

char EnvAA::PassID;

char EnvAAResultWrapperPass::ID = 0;
static RegisterPass<EnvAAResultWrapperPass> x("env-aa",
        "CPU environment struct alias analysis", false, true); 
//INITIALIZE_PASS_BEGIN(EnvAAResultWrapperPass, "anil-aa",
//                      "ObjC-ARC-Based Alias Analysis", false, true)
//INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
//INITIALIZE_PASS_END(EnvAAResultWrapperPass, "anil-aa",
//                    "ObjC-ARC-Based Alias Analysis", false, true)
//INITIALIZE_AG_PASS_BEGIN(EnvAAResultWrapperPass, AliasAnalysis, "anil-aa",
//                      "ObjC-ARC-Based Alias Analysis", false, true, false)
//INITIALIZE_PASS_DEPENDENCY(TargetLibraryInfoWrapperPass)
//INITIALIZE_AG_PASS_END(EnvAAResultWrapperPass, AliasAnalysis, "anil-aa",
//                    "ObjC-ARC-Based Alias Analysis", false, true, false)

//ImmutablePass *llvm::createEnvAAResultWrapperPass() {
//  return new EnvAAResultWrapperPass();
//}

EnvAAResultWrapperPass::EnvAAResultWrapperPass() : ImmutablePass(ID) {
  //initializeEnvAAResultWrapperPassPass(*PassRegistry::getPassRegistry());
}

bool EnvAAResultWrapperPass::doInitialization(Module &M) {
  Result.reset(new EnvAAResult(
      M.getDataLayout(), getAnalysis<TargetLibraryInfoWrapperPass>().getTLI()));
  return false;
}

bool EnvAAResultWrapperPass::doFinalization(Module &M) {
  Result.reset();
  return false;
}

void EnvAAResultWrapperPass::getAnalysisUsage(AnalysisUsage &AU) const {
  //AU.setPreservesAll();
  AU.addRequired<TargetLibraryInfoWrapperPass>();
}
