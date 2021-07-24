#include "implement_lib_call_stubs.hpp"
#include "pass_utils.hpp"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <llvm/IR/PassManager.h>
#include <map>

using namespace binrec;
using namespace llvm;
using namespace std;

// NOLINTNEXTLINE
auto ImplementLibCallStubsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    ifstream in;
    s2eOpen(in, "symbols");

    string symbol;
    unsigned addr = 0;
    map<string, unsigned> plt_map;

    while (in >> hex >> addr >> symbol)
        plt_map.insert(pair<string, unsigned>(symbol, addr));

    for (Function &f : m) {
        if (f.hasName() && f.getName().startswith("__stub")) {
            IRBuilder<> b(BasicBlock::Create(m.getContext(), "entry", &f));

            Value *target =
                b.CreateIntToPtr(b.getInt32(plt_map[f.getName().substr(7).str()]), f.getType());
            vector<Value *> args;
            transform(f.arg_begin(), f.arg_end(), back_inserter(args), [](Argument &a) {
                return &a;
            });
            CallInst *ret_val = b.CreateCall(f.getFunctionType(), target, args);
            ret_val->setTailCallKind(CallInst::TCK_MustTail);
            if (f.getReturnType()->isVoidTy()) {
                b.CreateRetVoid();
            } else {
                b.CreateRet(ret_val);
            }
        }
    }

    return PreservedAnalyses::none();
}
