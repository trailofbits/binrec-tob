#include "ReplaceStackVars.h"
#include "PassUtils.h"

using namespace llvm;

char ReplaceStackVars::ID = 0;
static RegisterPass<ReplaceStackVars> X("replace-stack-vars",
                                        "S2E Replace stack pointer accesses through R_EBP with array accesses "
                                        "to the global @stack",
                                        false, false);

auto ReplaceStackVars::runOnModule(Module &m) -> bool {
    GlobalVariable *R_EBP = m.getNamedGlobal("R_EBPx");
    assert(R_EBP && "@R_EBP does not exist");
    GlobalVariable *stack = m.getNamedGlobal("stack");
    assert(stack && "@stack does not exist");

    // For each memory GEP operand, check if the offset computation
    // involves a large constant in a known read-only data section
    for (User *use : R_EBP->users()) {
        if (auto *load = dyn_cast<LoadInst>(use)) {
            errs() << "load:" << *load << "\n";
            for (User *use : load->users()) {
                auto *inst = cast<Instruction>(use);
                { errs() << "  inst:" << *inst << "\n"; };
            }
        }
    }

    return true;
}
