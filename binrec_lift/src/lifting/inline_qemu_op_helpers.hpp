#ifndef BINREC_INLINE_QEMU_OP_HELPERS_HPP
#define BINREC_INLINE_QEMU_OP_HELPERS_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class InlineQemuOpHelpersPass : public llvm::PassInfoMixin<InlineQemuOpHelpersPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
