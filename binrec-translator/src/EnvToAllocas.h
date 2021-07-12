#ifndef BINREC_ENVTOALLOCAS_H
#define BINREC_ENVTOALLOCAS_H

#include <llvm/IR/PassManager.h>

namespace binrec {
/// S2E Transform environment globals into allocas in @wrapper
class EnvToAllocasPass : public llvm::PassInfoMixin<EnvToAllocasPass> {
public:
    auto run(llvm::Module &m, llvm::ModuleAnalysisManager &mam) -> llvm::PreservedAnalyses;
};
} // namespace binrec

#endif // BINREC_ENVTOALLOCAS_H
