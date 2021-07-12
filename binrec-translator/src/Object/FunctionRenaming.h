#ifndef BINREC_FUNCTIONRENAMING_H
#define BINREC_FUNCTIONRENAMING_H

#include <llvm/IR/PassManager.h>
#include <llvm/Object/ELFObjectFile.h>

namespace binrec {
class FunctionRenamingPass : public llvm::PassInfoMixin<FunctionRenamingPass> {
public:
    explicit FunctionRenamingPass();
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;

private:
    llvm::object::OwningBinary<llvm::object::Binary> binary;
};
} // namespace binrec

#endif // BINREC_FUNCTIONRENAMING_H
