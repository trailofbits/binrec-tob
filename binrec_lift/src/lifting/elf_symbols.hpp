#ifndef BINREC_ELF_SYMBOLS_HPP
#define BINREC_ELF_SYMBOLS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Add symbol metadata for ELF files based on stub addresses
    class ELFSymbolsPass : public llvm::PassInfoMixin<ELFSymbolsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
