#include "env_alias_analysis.hpp"
#include "ir/register.hpp"
#include <llvm/Analysis/TargetLibraryInfo.h>
#include <llvm/Analysis/ValueTracking.h>

using namespace binrec;
using namespace llvm;
using namespace std;

EnvAaResult::EnvAaResult(const DataLayout &dl) : dl(&dl) {}

// NOLINTNEXTLINE
auto EnvAaResult::invalidate(
    Function &f,
    const PreservedAnalyses &pa,
    FunctionAnalysisManager::Invalidator &inv) -> bool
{
    return false;
}

auto EnvAaResult::alias(const MemoryLocation &loc_a, const MemoryLocation &loc_b, AAQueryInfo &aaqi)
    -> LlvmAliasResult
{
    auto *a = dyn_cast<GlobalVariable>(getUnderlyingObject(loc_a.Ptr));
    auto *b = dyn_cast<GlobalVariable>(getUnderlyingObject(loc_b.Ptr));

    if (a &&
        (!a->hasName() ||
         find(Global_Emulation_Var_Names.begin(), Global_Emulation_Var_Names.end(), a->getName()) ==
             end(Global_Emulation_Var_Names)))
    {
        a = nullptr;
    }
    if (b &&
        (!b->hasName() ||
         find(Global_Emulation_Var_Names.begin(), Global_Emulation_Var_Names.end(), b->getName()) ==
             end(Global_Emulation_Var_Names)))
    {
        b = nullptr;
    }

    if (a && b) {
        return a == b ? LlvmAliasResult::MayAlias : LlvmAliasResult::NoAlias;
    }
    if (a || b) {
        return LlvmAliasResult::NoAlias;
    }
    return LlvmAliasResult::MayAlias;
}

static constexpr array<StringRef, 2> Safe_Intrinsics = {"llvm.stacksave", "llvm.stackrestore"};

auto EnvAaResult::getModRefInfo(const CallBase *call, const MemoryLocation &loc, AAQueryInfo &aaqi)
    -> ModRefInfo
{
    const Function *f = call->getCalledFunction();
    const Value *g = dyn_cast<GlobalVariable>(getUnderlyingObject(loc.Ptr));
    if (g &&
        (!g->hasName() ||
         find(Global_Emulation_Var_Names.begin(), Global_Emulation_Var_Names.end(), g->getName()) ==
             end(Global_Emulation_Var_Names)))
    {
        g = nullptr;
    }

    if (f && f->hasName() && g) {
        for (StringRef intrinsic : Safe_Intrinsics) {
            if (f->getName().startswith(intrinsic)) {
                return ModRefInfo::NoModRef;
            }
        }
    }
    return AAResultBase::getModRefInfo(call, loc, aaqi);
}

AnalysisKey EnvAa::Key;

// NOLINTNEXTLINE
auto EnvAa::run(Function &f, llvm::FunctionAnalysisManager &am) -> EnvAaResult
{
    return EnvAaResult(f.getParent()->getDataLayout());
}

char EnvAaResultWrapperPass::ID = 0;
static RegisterPass<EnvAaResultWrapperPass>
    base("env-aa", "CPU environment struct alias analysis", false, true);

EnvAaResultWrapperPass::EnvAaResultWrapperPass() : ImmutablePass(ID) {}

auto EnvAaResultWrapperPass::doInitialization(Module &m) -> bool
{
    result = make_unique<EnvAaResult>(m.getDataLayout());
    return false;
}

auto EnvAaResultWrapperPass::doFinalization(Module &m) -> bool
{
    result.reset();
    return false;
}

void EnvAaResultWrapperPass::getAnalysisUsage(AnalysisUsage &au) const
{
    au.addRequired<TargetLibraryInfoWrapperPass>();
}
