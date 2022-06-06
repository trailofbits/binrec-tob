#ifndef BINREC_FIX_CFG_WITHOUT_FUNC_DATA_HPP
#define BINREC_FIX_CFG_WITHOUT_FUNC_DATA_HPP

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Pass.h>
#include <map>
#include <unordered_set>

using namespace llvm;

struct fix_cfg_without_func_data : public ModulePass {
    static char ID;
    fix_cfg_without_func_data() : ModulePass(ID) {}

    auto runOnModule(Module &m) -> bool override;

private:
    void getBBs(Function &F, std::map<unsigned, BasicBlock *> &BBMap);
    void getBBsWithLibCalls(Function &F, std::unordered_set<BasicBlock *> &BBSet);
};

#endif
