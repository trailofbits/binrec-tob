/*
 * for each stub address in the symbols file, create an internal function with
 * that name that jumps to the corresponding PLT address
 */

#include "ELFPLTFuns.h"
#include "PassUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char ELFPLTFunsPass::ID = 0;
static RegisterPass<ELFPLTFunsPass> X("elf-plt-funs",
        "S2E Implement external function stubs with fallback to the PLT", false, false);

static bool implementPLTFun(Module &m, uint64_t addr, StringRef symbol) {
    Function *f = m.getFunction(symbol);

    if (!f)
        return false;

    f->setLinkage(GlobalValue::InternalLinkage);
    f->addFnAttr(Attribute::Naked);
    f->addFnAttr(Attribute::NoInline);

    IRBuilder<> b(BasicBlock::Create(m.getContext(), "entry", f));
    doInlineAsm(b, "pushl $0\n\tret", "i", true, b.getInt32(addr));
    b.CreateUnreachable();

    return true;
}

bool ELFPLTFunsPass::runOnModule(Module &m) {
    std::ifstream f;
    uint64_t addr;
    std::string symbol;
    bool changed = false;

    s2eOpen(f, "symbols");

    while (f >> std::hex >> addr >> symbol)
        changed |= implementPLTFun(m, addr, symbol);

    for (Function &f : m) {
        if (f.hasName() && f.empty() && !f.isIntrinsic() &&
                !f.getName().startswith("__st") &&
                !f.getName().startswith("__ld")) {
            errs() << "error: unresolved function: " << f.getName() << "\n";
            exit(1);
        }
    }

    return changed;
}
