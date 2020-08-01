#include "CyclomaticComplexity.h"
#include "PassUtils.h"

using namespace llvm;

char CyclomaticComplexity::ID = 0;
static RegisterPass<CyclomaticComplexity> x("cyclo",
        "S2E Measure cyclomatic complexity of recovered code",
        false, false);

bool CyclomaticComplexity::runOnModule(Module &m) {
    Function *f;

    if (!(f = m.getFunction("wrapper"))) {
        if (!(f = m.getFunction("main")))
            assert(false);
    }

    unsigned nodes = 0, edges = 0;

    for (BasicBlock &bb : *f) {
        TerminatorInst *term = bb.getTerminator();
        assert(term);
        edges += term->getNumSuccessors();
        nodes++;
    }

    unsigned complexity = edges - nodes + 2;
    INFO("Cyclomatic complexity = " << complexity <<
            " (" << edges << " edges - " << nodes << " nodes + 2)");

    return false;
}
