#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include <llvm/IR/CFG.h>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
auto byteWidth(Value &v) -> unsigned { return cast<IntegerType>(v.getType())->getBitWidth() / 8; }

auto findMemStore(Instruction *instStart) -> CallInst * {
    BasicBlock::iterator i(instStart);

    while (!isInstStart(&*(++i))) {
        if (auto *call = dyn_cast<CallInst>(i)) {
            Function *f = call->getCalledFunction();
            if (f && f->hasName() && f->getName().startswith("__st") && f->getName().endswith("_mmu")) {
                return call;
            }
        }
    }

    return nullptr;
}

auto getMemLoad(Instruction *i) -> CallInst * {
    if (auto *call = dyn_cast<CallInst>(i)) {
        Function *f = call->getCalledFunction();

        if (f && f->getName().startswith("__ld") && f->getName().endswith("_mmu")) {
            return call;
        }
    }
    return nullptr;
}

auto pruneArgs(Function &stub) -> unsigned {
    unsigned argsPruned = 0;
    std::vector<unsigned> argSizes;

    for (Argument &arg : stub.args())
        argSizes.push_back(byteWidth(arg));

    if (argSizes.empty())
        return 0;

    for (User *use : stub.users()) {
        auto *call = cast<CallInst>(use);
        BasicBlock *callee = call->getParent();

        // There should be exactly one predecessor
        auto ibegin = pred_begin(callee);
        auto iend = pred_end(callee);

        if (ibegin == iend) {
            INFO("ignore " << callee->getName() << " when removing libcall args (no predecesors)");
            continue;
        }

        assert(distance(ibegin, iend) == 1);

        unsigned nloads = 0;

        // Collect argument loads from stack before call
        std::vector<Instruction *> argLoads;
        BasicBlock::iterator i(call);

        while (--i != callee->begin()) {
            if (CallInst *memLoad = getMemLoad(&*i)) {
                argLoads.push_back(memLoad);

                if (++nloads == argSizes.size())
                    break;
            }
        }

        if (argLoads.size() != argSizes.size()) {
            errs() << "could not find all argument loads (" << argLoads.size() << " / " << argSizes.size() << "):";
            exit(1);
        }

        BasicBlock *caller = *pred_begin(callee);

        // Collect !inststart instructions in first order
        std::list<Instruction *> instStarts;

        for (Instruction &inst : *caller) {
            if (isInstStart(&inst))
                instStarts.push_front(&inst);
        }

        // Last instruction emulates the call
        instStarts.pop_front();

        std::list<Instruction *> pushInsts;

        for (Instruction *inst : instStarts) {
            if (CallInst *push = findMemStore(inst)) {
                pushInsts.push_front(push);

                if (pushInsts.size() == argSizes.size())
                    break;
            }
        }

        assert(pushInsts.size() == argSizes.size());

        // Collect stack stores in each push instruction before the call
        std::vector<Instruction *> eraseList;
        unsigned arg = 0;

        for (Instruction *push : pushInsts) {
            if (CallInst *otherStore = findMemStore(push)) {
                ERROR("multiple memory stores where one push was "
                      "expected:"
                      << *otherStore);
                exit(1);
            }

            const unsigned expectedSize = argSizes[arg];
            const unsigned storedSize = byteWidth(*push->getOperand(1));

            if (storedSize != expectedSize) {
                ERROR("expected arg push of " << expectedSize << " bytes, got " << storedSize << " bytes:" << *push);
                exit(1);
            }

            argLoads[arg]->replaceAllUsesWith(push->getOperand(1));
            eraseList.push_back(push);
            argsPruned++;
            arg++;
        }

        for (Instruction *inst : eraseList)
            inst->eraseFromParent();
    }

    return argsPruned;
}

/// S2E Remove stores to the stack before __stub_XXX calls
class PruneLibargsPushPass : public PassInfoMixin<PruneLibargsPushPass> {
public:
    /// From each (inlined) libcall to helper_stub_trampoline, if the libcall
    /// function is a __stub_XXX function, go to the only predecessor of the
    /// containing block. The number of arguments is known; for each argument, find
    /// the corresponding !inststart and from there find the (supposedly only, check
    /// this) store to any pointer not being @cc_op or @R_ESP and remove it.
    // NOLINTNEXTLINE
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        unsigned pruneCount = 0;

        for (Function &f : m) {
            if (f.hasName() && f.getName().startswith("__stub_"))
                pruneCount += pruneArgs(f);
        }

        DBG("pruned " << pruneCount << " pushed arguments at library calls");

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addPruneLibargsPushPass(ModulePassManager &mpm) { mpm.addPass(PruneLibargsPushPass()); }
