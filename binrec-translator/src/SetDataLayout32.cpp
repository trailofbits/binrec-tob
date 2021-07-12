#include "SetDataLayout32.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto SetDataLayout32Pass::run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
    m.setDataLayout("e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128");
    m.setTargetTriple("i386-pc-linux-gnu");
    return PreservedAnalyses::none();
}
