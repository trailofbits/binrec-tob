#include "MetaUtils.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;

namespace {
/// S2E Add debug print statements for PC values and illegal states
///
/// Assume the presence of a parameter-less function "helper_extern_call" that
/// that uses @R_ESP and @PC:
/// 1. reads a return address from///@R_ESP
/// 3. saves virtual values in concrete registers
/// 4. calls the external function at @PC
/// 5. saves concrete values in virtual registers
/// 6. jumps to the saved return address
class AddDebugPass : public PassInfoMixin<AddDebugPass> {
public:
    /// For each recovered basic block, find the first PC store and add a call to
    /// helper_debug_state
    // NOLINTNEXTLINE
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        if (debugVerbosity < 1)
            return PreservedAnalyses::all();

        LLVMContext &ctx = m.getContext();
        GlobalVariable *globalPc = m.getNamedGlobal("PC");
        assert(globalPc);

        for (Function &f : m) {
            if (!f.getName().startswith("Func_")) {
                continue;
            }

            auto *helperBreak = cast<Function>(m.getOrInsertFunction("helper_break", Type::getVoidTy(ctx)).getCallee());
            helperBreak->addFnAttr(Attribute::AlwaysInline);

            for (unsigned pc : breakAt) {
                bool found = false;

                for (BasicBlock &bb : f) {
                    if (isRecoveredBlock(&bb) && blockContains(&bb, pc)) {
                        CallInst::Create(helperBreak)->insertAfter(findInstStart(&bb, pc));
                        INFO("inserted breakpoint at " << intToHex(pc));
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    WARNING("could not find block containing " << intToHex(pc) << " to insert breakpoint");
                }
            }

            if (debugVerbosity >= 3) {
                auto *helperDebugState =
                    cast<Function>(m.getOrInsertFunction("helper_debug_state", Type::getVoidTy(ctx)).getCallee());

                for (BasicBlock &bb : f) {
                    if (isRecoveredBlock(&bb))
                        CallInst::Create(helperDebugState)->insertAfter(getFirstInstStart(&bb));
                }
            }
        }

        return PreservedAnalyses::allInSet<CFGAnalyses>();
    }
};
} // namespace

void binrec::addAddDebugPass(ModulePassManager &mpm) { mpm.addPass(AddDebugPass()); }
