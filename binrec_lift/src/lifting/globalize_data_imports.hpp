#ifndef BINREC_GLOBALIZE_DATA_IMPORTS_HPP
#define BINREC_GLOBALIZE_DATA_IMPORTS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class GlobalizeDataImportsPass : public llvm::PassInfoMixin<GlobalizeDataImportsPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
