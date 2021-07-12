/*
 * add a function 'void @mainCRTStartup()' which:
 * - allocates a stack buffer
 * - sets R_ESP to the end of the buffer
 * - calls wrapper()
 * - frees the stack buffer
 */

#include "PEMain.h"
#include "MainUtils.h"
#include "PassUtils.h"

#define MAIN_FUNCTION_NAME "mainCRTStartup"
#define TARGET_TRIPLE "i586-pc-win32-unknown"

using namespace llvm;

char PEMainPass::ID = 0;
static RegisterPass<PEMainPass> X("pe-main", "S2E Add main function that allocates stack and calls wrapper (PE)", false,
                                  false);

auto PEMainPass::runOnModule(Module &m) -> bool {
    m.setTargetTriple(TARGET_TRIPLE);

    LLVMContext &ctx = m.getContext();
    auto *main = cast<Function>(m.getOrInsertFunction(MAIN_FUNCTION_NAME, Type::getVoidTy(ctx)).getCallee());
    BasicBlock *bb = BasicBlock::Create(ctx, "entry", main);

    Value *stack = allocateStack(bb);
    callWrapper(bb);
    freeStack(stack, bb);
    ReturnInst::Create(ctx, bb);

    return true;
}
