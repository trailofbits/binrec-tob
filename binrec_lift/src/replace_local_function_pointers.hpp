#ifndef BINREC_REPLACE_LOCAL_FUNCTION_POINTERS_HPP
#define BINREC_REPLACE_LOCAL_FUNCTION_POINTERS_HPP

/*
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include <map>

using namespace llvm;

struct ReplaceLocalFunctionPointers : public FunctionPass {
    static char ID;
    ReplaceLocalFunctionPointers() : FunctionPass(ID) {}

    auto doInitialization(Module &m) -> bool override;
    auto runOnFunction(Function &f) -> bool override;

private:
    std::map<unsigned, BasicBlock *> allOriginalFunctions;
};
*/

#include <llvm/IR/PassManager.h>

namespace binrec {
    class ReplaceLocalFunctionPointers : public llvm::PassInfoMixin<ReplaceLocalFunctionPointers> {
    public:
        auto run(llvm::Module &m, llvm::ModuleAnalysisManager &am) -> llvm::PreservedAnalyses;
    };
} // namespace binrec

#endif
