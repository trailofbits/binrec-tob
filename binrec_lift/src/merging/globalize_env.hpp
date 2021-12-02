#ifndef BINREC_GLOBALIZE_ENV_HPP
#define BINREC_GLOBALIZE_ENV_HPP

#include <llvm/IR/PassManager.h>

namespace binrec {
    // Converts all references to the CPUX86State struct, either directly from the global
    // env-variable or from usage of CPUX86State * as first argument to functions (i.e. how 
    // translated blocks access it).
    // For each access, replace the access with access to a global variable representing the
    // CPUX86State field (split the struct into global variables for each field).
    // Prerequisites: RenameBlockFuncsPass have been run.
    class GlobalizeEnvPass : public llvm::PassInfoMixin<GlobalizeEnvPass> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
