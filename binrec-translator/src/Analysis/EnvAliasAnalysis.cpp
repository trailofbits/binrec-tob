#include "EnvAliasAnalysis.h"
#include "IR/Register.h"
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/ValueTracking.h>

using namespace binrec;
using namespace llvm;
using namespace std;

EnvAAResult::EnvAAResult(const DataLayout &dl) : dl(&dl) {}

// NOLINTNEXTLINE
auto EnvAAResult::invalidate(Function &f, const PreservedAnalyses &pa, FunctionAnalysisManager::Invalidator &inv)
    -> bool {
    return false;
}

auto EnvAAResult::alias(const MemoryLocation &locA, const MemoryLocation &locB, AAQueryInfo &aaqi) -> AliasResult {
    auto *a = dyn_cast<GlobalVariable>(getUnderlyingObject(locA.Ptr));
    auto *b = dyn_cast<GlobalVariable>(getUnderlyingObject(locB.Ptr));

    if (a && (!a->hasName() || find(globalEmulationVarNames.begin(), globalEmulationVarNames.end(), a->getName()) ==
                                   end(globalEmulationVarNames))) {
        a = nullptr;
    }
    if (b && (!b->hasName() || find(globalEmulationVarNames.begin(), globalEmulationVarNames.end(), b->getName()) ==
                                   end(globalEmulationVarNames))) {
        b = nullptr;
    }

    if (a && b) {
        return a == b ? MayAlias : NoAlias;
    }
    if (a || b) {
        return NoAlias;
    }
    return MayAlias;
}

static constexpr array<StringRef, 2> safeIntrinsics = {"llvm.stacksave", "llvm.stackrestore"};

auto EnvAAResult::getModRefInfo(const CallBase *call, const MemoryLocation &loc, AAQueryInfo &aaqi) -> ModRefInfo {
    const Function *f = call->getCalledFunction();
    const Value *g = dyn_cast<GlobalVariable>(getUnderlyingObject(loc.Ptr));
    if (g && (!g->hasName() || find(globalEmulationVarNames.begin(), globalEmulationVarNames.end(), g->getName()) ==
                                   end(globalEmulationVarNames))) {
        g = nullptr;
    }

    if (f && f->hasName() && g) {
        for (StringRef intrinsic : safeIntrinsics) {
            if (f->getName().startswith(intrinsic)) {
                return ModRefInfo::NoModRef;
            }
        }
    }
    return AAResultBase::getModRefInfo(call, loc, aaqi);
}

AnalysisKey EnvAA::Key;

// NOLINTNEXTLINE
auto EnvAA::run(Function &f, llvm::FunctionAnalysisManager &am) -> EnvAAResult {
    return EnvAAResult(f.getParent()->getDataLayout());
}

char EnvAAResultWrapperPass::ID = 0;
static RegisterPass<EnvAAResultWrapperPass> x("env-aa", "CPU environment struct alias analysis", false, true);

EnvAAResultWrapperPass::EnvAAResultWrapperPass() : ImmutablePass(ID) {}

auto EnvAAResultWrapperPass::doInitialization(Module &m) -> bool {
    result = make_unique<EnvAAResult>(m.getDataLayout());
    return false;
}

auto EnvAAResultWrapperPass::doFinalization(Module &m) -> bool {
    result.reset();
    return false;
}

void EnvAAResultWrapperPass::getAnalysisUsage(AnalysisUsage &au) const {
    au.addRequired<TargetLibraryInfoWrapperPass>();
}
