#include "add_debug.hpp"
#include "error.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "pc_utils.hpp"
#include <llvm/IR/PassManager.h>

#define PASS_NAME "add_debug"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;

// NOLINTNEXTLINE
auto AddDebugPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    if (debugVerbosity < 1)
        return PreservedAnalyses::all();

    LLVMContext &ctx = m.getContext();
    GlobalVariable *global_pc = m.getNamedGlobal("PC");
    PASS_ASSERT(global_pc);

    for (Function &f : m) {
        if (!f.getName().startswith("Func_")) {
            continue;
        }

        auto *helper_break =
            cast<Function>(m.getOrInsertFunction("helper_break", Type::getVoidTy(ctx)).getCallee());
        helper_break->addFnAttr(Attribute::AlwaysInline);

        for (unsigned pc : breakAt) {
            bool found = false;

            for (BasicBlock &bb : f) {
                if (isRecoveredBlock(&bb) && blockContains(&bb, pc)) {
                    CallInst::Create(helper_break)->insertAfter(findInstStart(&bb, pc));
                    INFO("inserted breakpoint at " << utohexstr(pc));
                    found = true;
                    break;
                }
            }

            if (!found) {
                WARNING(
                    "could not find block containing " << utohexstr(pc) << " to insert breakpoint");
            }
        }

        if (debugVerbosity >= 3) {
            auto *helper_debug_state = cast<Function>(
                m.getOrInsertFunction("helper_debug_state", Type::getVoidTy(ctx)).getCallee());

            for (BasicBlock &bb : f) {
                if (isRecoveredBlock(&bb))
                    CallInst::Create(helper_debug_state)->insertAfter(getFirstInstStart(&bb));
            }
        }
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
