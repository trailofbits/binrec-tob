#include <map>
#include <iostream>

#include "ImplementLibCallStubs.h"
#include "PassUtils.h"

using namespace llvm;

char ImplementLibCallStubs::ID = 0;
static RegisterPass<ImplementLibCallStubs> X("implement-lib-call-stubs",
        "S2E Insert PLT jumps in library function stubs",
        false, false);

bool ImplementLibCallStubs::runOnModule(Module &m) {
    std::ifstream f;
    s2eOpen(f, "symbols");

    std::string symbol;
    unsigned addr;
    std::map<std::string, unsigned> pltmap;

    while (f >> std::hex >> addr >> symbol)
        pltmap.insert(std::pair<std::string, unsigned>(symbol, addr));

    for (Function &f : m) {
        if (f.hasName() && f.getName().startswith("__stub")) {
            f.addFnAttr(Attribute::NoReturn);
            IRBuilder<> b(BasicBlock::Create(m.getContext(), "entry", &f));

            doInlineAsm(b, "pushl $0\n\tret", "i", false,
                    b.getInt32(pltmap[f.getName().substr(7)]));
            b.CreateUnreachable();
        }
    }

    return true;
}
