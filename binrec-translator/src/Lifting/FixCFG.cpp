#include "Analysis/TraceInfoAnalysis.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include "Utils/FunctionInfo.h"
#include <llvm/IR/CFG.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
/// Fix cfg by finding escaping pointers
///
/// This pass works only if library functions that take local pointer are reached
/// by call instruction. If library function is reached by jmp instruction, this
/// pass will fail.
class FixCFGPass : public PassInfoMixin<FixCFGPass> {
public:
    // NOLINTNEXTLINE
    auto run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
        TraceInfo &ti = mam.getResult<TraceInfoAnalysis>(m);
        FunctionInfo fi{ti};

        for (Function &F : m) {
            if (!F.getName().startswith("Func_")) {
                continue;
            }

            unordered_set<BasicBlock *> BBSet;
            getBBsWithLibCalls(F, BBSet);

            map<unsigned, BasicBlock *> BBMap;
            getBBs(F, BBMap);

            unordered_map<unsigned, unordered_set<BasicBlock *>> callerPcToCallerBB;
            findCallerBBs(F, callerPcToCallerBB, fi);

            list<pair<SwitchInst *, BasicBlock *>> updateList;
            unordered_map<BasicBlock *, unordered_set<BasicBlock *>> jmpBBs;
            // unordered_map<BasicBlock *, unordered_set<BasicBlock *> >
            // callBBs;

            for (BasicBlock *B : BBSet) {
                // find caller of this function
                unsigned pc = getBlockAddress(B);
                auto callerPcSet = fi.entryPcToCallerPcs[pc];

                // check if each pred is callerBB
                // if it is, then it function called by call inst
                // otherwise, it is called by jmp
                for (auto P = pred_begin(B), E = pred_end(B); P != E; ++P) {
                    if (pred_begin(*P) == pred_end(*P) && &(F.getEntryBlock()) != *P)
                        continue;
                    MDNode *md = getBlockMeta(*P, "inlined_lib_func");
                    if (md) {
                        continue;
                    }

                    bool found = false;
                    for (unsigned callerPc : callerPcSet) {
                        for (auto *callerBB : callerPcToCallerBB[callerPc]) {
                            if (callerBB == *P) {
                                // callBBs[B].insert(*P);
                                found = true;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }
                    if (!found && BBSet.find(*P) == BBSet.end()) {
                        // MDNode *md = getBlockMeta(*P, "inlined_lib_func");
                        // if (!md){
                        jmpBBs[B].insert(*P);
                        //}
                    }
                }
            }

            for (auto &p : jmpBBs) {
                DBG("BB: " << p.first->getName());
                for (BasicBlock *B : p.second) {
                    DBG("Jump BB: " << B->getName());
                }
                DBG("-----------------");
            }

            unordered_map<BasicBlock *, unordered_set<unsigned>> newSuccs;

            // Add new succs for jmp blocks
            DBG("-----newSuccs-Jump------");
            for (auto &p : jmpBBs) {
                DBG("BB: " << p.first->getName());
                for (BasicBlock *B : p.second) {
                    // func that called func(p.first) with jmp
                    unsigned BPc = getBlockAddress(B);
                    DBG("BB_pred_pc: " << intToHex(BPc));
                    for (unsigned callerFuncPc : fi.BBPcToEntryPcs[BPc]) {
                        DBG("caller func: " << intToHex(callerFuncPc));
                        for (unsigned callerOfCallerFunc : fi.entryPcToCallerPcs[callerFuncPc]) {
                            unsigned followUp = fi.callerPcToFollowUpPc[callerOfCallerFunc];
                            DBG("follow_up to be added: " << intToHex(followUp));
                            newSuccs[p.first].insert(followUp);
                        }
                    }
                }
                DBG("-----------------");
            }

            // Add new succs for call blocks
            DBG("-----newSuccs-Call------");
            for (BasicBlock *B : BBSet) {
                DBG("BB: " << B->getName());
                // find caller of this function
                unsigned pc = getBlockAddress(B);
                auto callerPcSet = fi.entryPcToCallerPcs[pc];

                // check if each pred is callerBB
                // if it is, then it function called by call inst
                // otherwise, it is called by jmp
                for (auto P = pred_begin(B), E = pred_end(B); P != E; ++P) {
                    if (pred_begin(*P) == pred_end(*P) && &(F.getEntryBlock()) != *P)
                        continue;
                    MDNode *md = getBlockMeta(*P, "inlined_lib_func");
                    if (md) {
                        continue;
                    }

                    bool found = false;
                    for (unsigned callerPc : callerPcSet) {
                        for (auto *callerBB : callerPcToCallerBB[callerPc]) {
                            if (callerBB == *P) {
                                unsigned followUp = fi.callerPcToFollowUpPc[callerPc];
                                DBG("BB_pred_pc: " << callerBB->getName());
                                DBG("caller_pc: " << callerPc << " followUp: " << intToHex(followUp));
                                newSuccs[B].insert(followUp);
                                found = true;
                                break;
                            }
                        }
                        if (found)
                            break;
                    }
                }
                DBG("-----------------");
            }

            for (auto &p : updateList) {
                ConstantInt *addr =
                    ConstantInt::get(Type::getInt32Ty(p.second->getContext()), getBlockAddress(p.second));

                p.first->addCase(addr, p.second);
            }

            addSwitchCases(BBMap, newSuccs);
        }

        return PreservedAnalyses::none();
    }

private:
    auto hasPc(BasicBlock *bb, GlobalVariable *PC, uint32_t pc) -> bool {
        for (Instruction &inst : *bb) {
            if (auto *store = dyn_cast<StoreInst>(&inst)) {
                if (PC == store->getPointerOperand()) {
                    if (auto *storePc = dyn_cast<ConstantInt>(store->getValueOperand())) {
                        if (storePc->getZExtValue() == pc) {
                            return true;
                        }
                    }
                }
            }
        }

        return false;
    }

    void findCallerBBs(Function &F, unordered_map<unsigned, unordered_set<BasicBlock *>> &callerPcToCallerBB,
                       const FunctionInfo &fi) {
        GlobalVariable *PC = F.getParent()->getNamedGlobal("PC");

        for (auto &p : fi.callerPcToFollowUpPc) {
            for (BasicBlock &B : F) {
                if (!hasPc(&B, PC, p.first))
                    continue;

                callerPcToCallerBB[p.first].insert(&B);
                // assert(callerPcToCallerBB.insert({p.first, &B}).second && "Duplicate
                // BB for the same pc");
                /*
                if(callerPcToCallerBB.insert({p.first, &B}).second == false){
                    DBG("Duplicate:");
                    DBG("caller_pc: " << p.first);
                    DBG(callerPcToCallerBB[p.first]->getName());
                    DBG(B.getName());
                }
                */
            }
            // assert(callerPcToCallerBB.find(p.first) != callerPcToCallerBB.end() &&
            // "No BB for caller");
        }

        DBG("___CallerPcToCallerBB");
        for (auto &p : callerPcToCallerBB) {
            for (auto *B : p.second) {
                DBG("caller_pc: " << p.first << " callerBB: " << B->getName());
            }
        }
    }

    void getBBs(Function &F, map<unsigned, BasicBlock *> &BBMap) {
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

    void getBBsWithLibCalls(Function &F, unordered_set<BasicBlock *> &BBSet) {
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

    void addSwitchCases(map<unsigned, BasicBlock *> &BBMap,
                        unordered_map<BasicBlock *, unordered_set<unsigned>> &newSuccs) {

        for (auto &p : newSuccs) {
            for (auto S = succ_begin(p.first), E = succ_end(p.first); S != E; ++S) {
                p.second.erase(getBlockAddress(*S));
            }

            if (p.second.empty()) {
                continue;
            }

            for (unsigned succPc : p.second) {
                BasicBlock *succBB = BBMap[succPc];
                auto *sw = dyn_cast<SwitchInst>(p.first->getTerminator());
                if (sw == nullptr) {
                    continue;
                }
                ConstantInt *addr = ConstantInt::get(Type::getInt32Ty(succBB->getContext()), succPc);

                // add the case if doesn't exist
                if (sw->findCaseValue(addr) == sw->case_default()) {
                    // updateList.push_back({sw, nextBB});
                    sw->addCase(addr, succBB);
                    // DBG(*sw);
                }
            }
        }
    }
};
} // namespace

void binrec::addFixCFGPass(ModulePassManager &mpm) { return mpm.addPass(FixCFGPass{}); }
