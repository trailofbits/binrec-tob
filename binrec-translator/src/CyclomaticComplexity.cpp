#include "CyclomaticComplexity.h"
#include "PassUtils.h"

using namespace llvm;

char CyclomaticComplexity::ID = 0;
static RegisterPass<CyclomaticComplexity> x("cyclo", "S2E Measure cyclomatic complexity of recovered code", false,
                                            false);

auto CyclomaticComplexity::runOnModule(Module &m) -> bool {
    Function *f = nullptr;

    if (!(f = m.getFunction("Func_wrapper"))) {
        if (!(f = m.getFunction("main")))
            assert(false);
    }

    unsigned nodes = 0, edges = 0;

    for (BasicBlock &bb : *f) {
        Instruction *term = bb.getTerminator();
        assert(term);
        edges += term->getNumSuccessors();
        nodes++;
    }

    unsigned complexity = edges - nodes + 2;
    INFO("Cyclomatic complexity = " << complexity << " (" << edges << " edges - " << nodes << " nodes + 2)");

    return false;
}
