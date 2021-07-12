#include "PassUtils.h"
#include "RegisterPasses.h"
#include <algorithm>
#include <fstream>
#include <iterator>
#include <llvm/IR/PassManager.h>
#include <map>

using namespace llvm;

namespace {
/// S2E Insert PLT jumps in library function stubs
class ImplementLibCallStubsPass : public PassInfoMixin<ImplementLibCallStubsPass> {
public:
    auto run(Module &M, ModuleAnalysisManager &) -> PreservedAnalyses {
        std::ifstream F;
        s2eOpen(F, "symbols");

        std::string Symbol;
        unsigned Addr = 0;
        std::map<std::string, unsigned> PLTMap;

        while (F >> std::hex >> Addr >> Symbol)
            PLTMap.insert(std::pair<std::string, unsigned>(Symbol, Addr));

        for (Function &F : M) {
            if (F.hasName() && F.getName().startswith("__stub")) {
                IRBuilder<> B(BasicBlock::Create(M.getContext(), "entry", &F));

                Value *Target = B.CreateIntToPtr(B.getInt32(PLTMap[F.getName().substr(7).str()]), F.getType());
                std::vector<Value *> Args;
                std::transform(F.arg_begin(), F.arg_end(), std::back_inserter(Args), [](Argument &A) { return &A; });
                CallInst *RetVal = B.CreateCall(F.getFunctionType(), Target, Args);
                RetVal->setTailCallKind(CallInst::TCK_MustTail);
                if (F.getReturnType()->isVoidTy()) {
                    B.CreateRetVoid();
                } else {
                    B.CreateRet(RetVal);
                }
            }
        }

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addImplementLibCallStubsPass(ModulePassManager &mpm) { return mpm.addPass(ImplementLibCallStubsPass()); }
