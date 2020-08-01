/*
 * Assume the presence of a parameter-less function "helper_extern_call" that
 * that uses @R_ESP and @PC:
 * 1. reads a return address from *@R_ESP
 * 3. saves virtual values in concrete registers
 * 4. calls the external function at @PC
 * 5. saves concrete values in virtual registers
 * 6. jumps to the saved return address
 */

#include "AddDebug.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "DebugUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char AddDebug::ID = 0;
static RegisterPass<AddDebug> X("add-debug-output",
        "S2E Add debug print statements for PC values and illegal states", false, false);

/*
 * For each recovered basic block, find the first PC store and add a call to
 * helper_debug_state
 */
bool AddDebug::runOnModule(Module &m) {
    if (debugVerbosity < 1)
        return false;

    LLVMContext &ctx = m.getContext();
    GlobalVariable *PC = m.getNamedGlobal("PC");
    assert(PC);

    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper);

    Function *helper_break = cast<Function>(m.getOrInsertFunction(
                "helper_break", Type::getVoidTy(ctx), NULL));
    helper_break->addFnAttr(Attribute::AlwaysInline);

    for (unsigned pc : breakAt) {
        bool found = false;

        for (BasicBlock &bb : *wrapper) {
            if (isRecoveredBlock(&bb) && !getBlockMeta(&bb, "lastpc")) {
                bb.dump();
            }
            if (isRecoveredBlock(&bb) && blockContains(&bb, pc)) {
                CallInst::Create(helper_break)->insertAfter(findInstStart(&bb, pc));
                INFO("inserted breakpoint at " << intToHex(pc));
                found = true;
                break;
            }
        }

        if (!found) {
            WARNING("could not find block containing " << intToHex(pc) <<
                    " to insert breakpoint");
        }
    }

    if (debugVerbosity >= 3) {
        Function *helper_debug_state = cast<Function>(m.getOrInsertFunction(
                    "helper_debug_state", Type::getVoidTy(ctx), NULL));

        for (BasicBlock &bb : *wrapper) {
            if (isRecoveredBlock(&bb))
                CallInst::Create(helper_debug_state)->insertAfter(getFirstInstStart(&bb));
        }
    }

    return true;
}
