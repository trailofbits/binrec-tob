#ifndef _ENV_ALIAS_ANALYSIS_H
#define _ENV_ALIAS_ANALYSIS_H

#include "llvm/Analysis/AliasAnalysis.h"
#include "llvm/Analysis/TargetLibraryInfo.h"
#include "llvm/Pass.h"

namespace llvm {

/// \brief This is a simple alias analysis implementation that uses knowledge
/// of ARC constructs to answer queries.
///
/// TODO: This class could be generalized to know about other ObjC-specific
/// tricks. Such as knowing that ivars in the non-fragile ABI are non-aliasing
/// even though their offsets are dynamic.
class EnvAAResult : public AAResultBase<EnvAAResult> {
  friend AAResultBase<EnvAAResult>;

  const DataLayout &DL;

public:
  explicit EnvAAResult(const DataLayout &DL, const TargetLibraryInfo &TLI)
      : AAResultBase(TLI), DL(DL) {}
  EnvAAResult(EnvAAResult &&Arg)
      : AAResultBase(std::move(Arg)), DL(Arg.DL) {}

  /// Handle invalidation events from the new pass manager.
  ///
  /// By definition, this result is stateless and so remains valid.
  bool invalidate(Function &, const PreservedAnalyses &) { return false; }

  AliasResult alias(const MemoryLocation &LocA, const MemoryLocation &LocB);
  //bool pointsToConstantMemory(const MemoryLocation &Loc, bool OrLocal);

  //using AAResultBase::getModRefBehavior;
  //FunctionModRefBehavior getModRefBehavior(const Function *F);

  using AAResultBase::getModRefInfo;
  ModRefInfo getModRefInfo(ImmutableCallSite CS, const MemoryLocation &Loc);
};

/// Analysis pass providing a never-invalidated alias analysis result.
class EnvAA {
public:
  typedef EnvAAResult Result;

  /// \brief Opaque, unique identifier for this analysis pass.
  static void *ID() { return (void *)&PassID; }

  EnvAAResult run(Function &F, AnalysisManager<Function> *AM);

  /// \brief Provide access to a name for this pass for debugging purposes.
  static StringRef name() { return "EnvAAResult"; }

private:
  static char PassID;
};

/// Legacy wrapper pass to provide the ObjCARCAAResult object.
class EnvAAResultWrapperPass : public ImmutablePass {
  std::unique_ptr<EnvAAResult> Result;

public:
  static char ID;

  EnvAAResultWrapperPass();

  EnvAAResult &getResult() { return *Result; }
  const EnvAAResult &getResult() const { return *Result; }

  bool doInitialization(Module &M) override;
  bool doFinalization(Module &M) override;
  void getAnalysisUsage(AnalysisUsage &AU) const override;
};

} // namespace llvm

#endif
