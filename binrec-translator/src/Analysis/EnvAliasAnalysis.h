#ifndef BINREC_ENVALIASANALYSIS_H
#define BINREC_ENVALIASANALYSIS_H

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/PassManager.h>

namespace binrec {

class EnvAAResult : public llvm::AAResultBase<EnvAAResult> {
    friend AAResultBase<EnvAAResult>;

    const llvm::DataLayout *dl;

public:
    explicit EnvAAResult(const llvm::DataLayout &dl);

    /// Handle invalidation events from the new pass manager.
    ///
    /// By definition, this result is stateless and so remains valid.
    auto invalidate(llvm::Function &f, const llvm::PreservedAnalyses &pa,
                    llvm::FunctionAnalysisManager::Invalidator &inv) -> bool;

    auto alias(const llvm::MemoryLocation &locA, const llvm::MemoryLocation &locB, llvm::AAQueryInfo &aaqi)
        -> llvm::AliasResult;
    using AAResultBase::getModRefInfo;
    auto getModRefInfo(const llvm::CallBase *call, const llvm::MemoryLocation &loc, llvm::AAQueryInfo &aaqi)
        -> llvm::ModRefInfo;
};

/// Analysis pass providing a never-invalidated alias analysis result.
class EnvAA : public llvm::AnalysisInfoMixin<EnvAA> {
    friend llvm::AnalysisInfoMixin<EnvAA>;
    static llvm::AnalysisKey Key; // NOLINT

public:
    using Result = EnvAAResult;
    auto run(llvm::Function &f, llvm::FunctionAnalysisManager &am) -> EnvAAResult;
};

/// Legacy wrapper pass to provide the ObjCARCAAResult object.
class EnvAAResultWrapperPass : public llvm::ImmutablePass {
    std::unique_ptr<EnvAAResult> result;

public:
    static char ID; // NOLINT

    EnvAAResultWrapperPass();

    auto getResult() -> EnvAAResult & { return *result; }
    [[nodiscard]] auto getResult() const -> const EnvAAResult & { return *result; }

    auto doInitialization(llvm::Module &m) -> bool override;
    auto doFinalization(llvm::Module &m) -> bool override;
    void getAnalysisUsage(llvm::AnalysisUsage &au) const override;
};

} // namespace binrec

#endif
