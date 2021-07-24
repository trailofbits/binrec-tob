#ifndef BINREC_REMOVE_METADATA_HPP
#define BINREC_REMOVE_METADATA_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    /// S2E Remove all unused metadata if not in debug mode
    class RemoveMetadataPass : public llvm::PassInfoMixin<RemoveMetadataPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
