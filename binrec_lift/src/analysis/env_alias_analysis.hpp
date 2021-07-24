#ifndef BINREC_ENV_ALIAS_ANALYSIS_HPP
#define BINREC_ENV_ALIAS_ANALYSIS_HPP

#include <llvm/Analysis/AliasAnalysis.h>
#include <llvm/IR/PassManager.h>

namespace binrec {

    class EnvAaResult : public llvm::AAResultBase<EnvAaResult> {
        friend AAResultBase<EnvAaResult>;

        const llvm::DataLayout *dl;

    public:
        explicit EnvAaResult(const llvm::DataLayout &dl);

        /// Handle invalidation events from the new pass manager.
        ///
        /// By definition, this result is stateless and so remains valid.
        auto invalidate(
            llvm::Function &f,
            const llvm::PreservedAnalyses &pa,
            llvm::FunctionAnalysisManager::Invalidator &inv) -> bool;

        auto alias(
            const llvm::MemoryLocation &loc_a,
            const llvm::MemoryLocation &loc_b,
            llvm::AAQueryInfo &aaqi) -> llvm::AliasResult;
        using AAResultBase::getModRefInfo;
        auto getModRefInfo(
            const llvm::CallBase *call,
            const llvm::MemoryLocation &loc,
            llvm::AAQueryInfo &aaqi) -> llvm::ModRefInfo;
    };

    /// Analysis pass providing a never-invalidated alias analysis result.
    class EnvAa : public llvm::AnalysisInfoMixin<EnvAa> {
        friend llvm::AnalysisInfoMixin<EnvAa>;
        static llvm::AnalysisKey Key; // NOLINT

    public:
        using Result = EnvAaResult;
        auto run(llvm::Function &f, llvm::FunctionAnalysisManager &am) -> EnvAaResult;
    };

    /// Legacy wrapper pass to provide the ObjCARCAAResult object.
    class EnvAaResultWrapperPass : public llvm::ImmutablePass {
        std::unique_ptr<EnvAaResult> result;

    public:
        static char ID; // NOLINT

        EnvAaResultWrapperPass();

        // NOLINTNEXTLINE
        auto getResult() -> EnvAaResult &
        {
            return *result;
        }
        // NOLINTNEXTLINE
        [[nodiscard]] auto getResult() const -> const EnvAaResult &
        {
            return *result;
        }

        auto doInitialization(llvm::Module &m) -> bool override;
        auto doFinalization(llvm::Module &m) -> bool override;
        void getAnalysisUsage(llvm::AnalysisUsage &au) const override;
    };

} // namespace binrec

#endif
