#include "ReplaceStackVars.h"
#include "PassUtils.h"

using namespace llvm;

char ReplaceStackVars::ID = 0;
static RegisterPass<ReplaceStackVars> X("replace-stack-vars",
        "S2E Replace stack pointer accesses through R_EBP with array accesses "
        "to the global @stack", false, false);


bool ReplaceStackVars::runOnModule(Module &m) {
    GlobalVariable *R_EBP = m.getNamedGlobal("R_EBPx");
    assert(R_EBP && "@R_EBP does not exist");
    GlobalVariable *stack = m.getNamedGlobal("stack");
    assert(stack && "@stack does not exist");

    // For each memory GEP operand, check if the offset computation
    // involves a large constant in a known read-only data section
    foreach_use_if(R_EBP, LoadInst, load, {
        errs() << "load:" << *load << "\n";
        foreach_use_as(load, Instruction, inst, {
            errs() << "  inst:" << *inst << "\n";
        });
    });

    return true;;
}
