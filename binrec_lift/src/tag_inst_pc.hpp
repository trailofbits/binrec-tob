#ifndef BINREC_TAG_INST_PC_HPP
#define BINREC_TAG_INST_PC_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Add metadata annotations to PC stores at instruction boundaries
    class TagInstPcPass : public llvm::PassInfoMixin<TagInstPcPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
