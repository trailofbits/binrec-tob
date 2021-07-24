#ifndef BINREC_REPLACE_DYNAMIC_SYMBOLS_HPP
#define BINREC_REPLACE_DYNAMIC_SYMBOLS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// parse readelf -D -s to get the dynamic object symbols (not functions), then if we find one
    /// of those addresses as an immmediate replace it with the symbol name
    class ReplaceDynamicSymbolsPass : public llvm::PassInfoMixin<ReplaceDynamicSymbolsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
