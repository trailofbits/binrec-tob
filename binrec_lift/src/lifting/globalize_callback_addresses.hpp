#ifndef BINREC_GLOBALIZE_CALLBACK_ADDRESSES_HPP
#define BINREC_GLOBALIZE_CALLBACK_ADDRESSES_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    class GlobalizeCallbackAddresses : public llvm::PassInfoMixin<GlobalizeCallbackAddresses> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
