#ifndef BINREC_FUNCTION_RENAMING_HPP
#define BINREC_FUNCTION_RENAMING_HPP

#include <llvm/IR/PassManager.h>
#include <llvm/Object/ELFObjectFile.h>

namespace binrec {
    class FunctionRenamingPass : public llvm::PassInfoMixin<FunctionRenamingPass> {
    public:
        explicit FunctionRenamingPass();
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;

    private:
        llvm::object::OwningBinary<llvm::object::Binary> binary;
    };
} // namespace binrec

#endif
