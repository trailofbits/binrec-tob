#include "FixCFGWithoutFuncData.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include <algorithm>
#include <llvm/IR/CFG.h>

using namespace llvm;

char FixCFGWithoutFuncData::ID = 0;
static RegisterPass<FixCFGWithoutFuncData> X("fix-cfg-without-func-data", "Fix cfg by finding escaping pointers", false,
                                             false);

// This pass works only if library functions that take local pointer are reached by call instruction.
// If library function is reached by jmp instruction, this pass will fail.

void FixCFGWithoutFuncData::getBBs(Function &F, std::map<unsigned, BasicBlock *> &BBMap) {
    BasicBlock &entryBB = F.getEntryBlock();
    for (BasicBlock &B : F) {
        if (!isRecoveredBlock(&B))
            continue;
        // Discard BBs that has no preds
        if (pred_begin(&B) == pred_end(&B) && &B != &entryBB) {
            DBG("Discard BB: " << B.getName());
            continue;
        }
        unsigned pc = getBlockAddress(&B);
        BBMap[pc] = &B;
        DBG("BB pc: " << intToHex(pc) << ":" << pc << " BB: " << B.getName());
    }
}

// TODO: check BB_8049b70.from_BB_8049aa0 and BB_8049b70
void FixCFGWithoutFuncData::getBBsWithLibCalls(Function &F, std::unordered_set<BasicBlock *> &BBSet) {
    BasicBlock &entryBB = F.getEntryBlock();
    for (BasicBlock &B : F) {
        if (!isRecoveredBlock(&B))
            continue;
        // Discard BBs that has no preds
        if (pred_begin(&B) == pred_end(&B) && &B != &entryBB) {
            DBG("Discard BB: " << B.getName());
            continue;
        }
        for (Instruction &I : B) {
            if (!I.getMetadata("funcname"))
                continue;
            BBSet.insert(&B);
            DBG("BB with Func Call: " << B.getName());
            break;
        }
    }
}

auto FixCFGWithoutFuncData::runOnModule(Module &m) -> bool {
    Function *wrapper = m.getFunction("Func_wrapper");
    assert(wrapper && "No wrapper Function");

    std::unordered_set<BasicBlock *> BBSet;
    getBBsWithLibCalls(*wrapper, BBSet);

    std::map<unsigned, BasicBlock *> BBMap;
    getBBs(*wrapper, BBMap);

    std::list<std::pair<SwitchInst *, BasicBlock *>> updateList;

    for (BasicBlock *B : BBSet) {
        unsigned pc = getBlockAddress(*pred_begin(B));
        auto it = BBMap.find(pc);
        ++it;
        BasicBlock *nextBB = it->second;
        bool addCase = true;
        for (auto S = succ_begin(B), E = succ_end(B); S != E; S++) {
            BasicBlock *succBB = *S;
            if (succBB == nextBB) {
                addCase = false;
                break;
            }
        }

        DBG("libCallBB: " << B->getName() << " predBB: " << (*pred_begin(B))->getName()
                          << " nextBB: " << nextBB->getName() << " addCase: " << addCase);
        if (addCase) {
            auto *sw = dyn_cast<SwitchInst>(B->getTerminator());
            assert(sw && "Terminator is not switch inst");
            ConstantInt *addr = ConstantInt::get(Type::getInt32Ty(B->getContext()), getBlockAddress(nextBB));

            // add the case if doesn't exist
            if (sw->findCaseValue(addr) == sw->case_default()) {
                updateList.emplace_back(sw, nextBB);
                // sw->addCase(addr, nextBB);
                DBG(*sw);
            }
        }
    }

    for (auto &p : updateList) {
        ConstantInt *addr = ConstantInt::get(Type::getInt32Ty(p.second->getContext()), getBlockAddress(p.second));

        p.first->addCase(addr, p.second);
    }
    return true;
}
