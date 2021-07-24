#ifndef BINREC_SET_DATA_LAYOUT_32_HPP
#define BINREC_SET_DATA_LAYOUT_32_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class SetDataLayout32Pass : public llvm::PassInfoMixin<SetDataLayout32Pass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
