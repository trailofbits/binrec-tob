#include "fix_cfg.hpp"
#include "analysis/trace_info_analysis.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "utils/function_info.hpp"
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

static auto has_pc(BasicBlock *bb, GlobalVariable *global_pc, uint32_t pc) -> bool
{
    for (Instruction &inst : *bb) {
        if (auto *store = dyn_cast<StoreInst>(&inst)) {
            if (global_pc == store->getPointerOperand()) {
                if (auto *store_pc = dyn_cast<ConstantInt>(store->getValueOperand())) {
                    if (store_pc->getZExtValue() == pc) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

static void find_caller_bbs(
    Function &f,
    unordered_map<unsigned, unordered_set<BasicBlock *>> &caller_pc_to_caller_bb,
    const FunctionInfo &fi)
{
    GlobalVariable *pc = f.getParent()->getNamedGlobal("PC");

    for (auto &p : fi.caller_pc_to_follow_up_pc) {
        for (BasicBlock &bb : f) {
            if (!has_pc(&bb, pc, p.first))
                continue;

            caller_pc_to_caller_bb[p.first].insert(&bb);
            // PASS_ASSERT(callerPcToCallerBB.insert({p.first, &B}).second && "Duplicate
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
        // PASS_ASSERT(callerPcToCallerBB.find(p.first) != callerPcToCallerBB.end() &&
        // "No BB for caller");
    }

    DBG("___CallerPcToCallerBB");
    for (auto &p : caller_pc_to_caller_bb) {
        for (auto *bb : p.second) {
            DBG("caller_pc: " << p.first << " callerBB: " << bb->getName());
        }
    }
}

static void get_bbs(Function &f, map<unsigned, BasicBlock *> &bb_map)
{
    BasicBlock &entry_bb = f.getEntryBlock();
    for (BasicBlock &bb : f) {
        if (!isRecoveredBlock(&bb))
            continue;
        // Discard BBs that has no preds
        if (pred_empty(&bb) && &bb != &entry_bb) {
            DBG("Discard BB: " << bb.getName());
            continue;
        }
        unsigned pc = getBlockAddress(&bb);
        bb_map[pc] = &bb;
        DBG("BB pc: " << utohexstr(pc) << ":" << pc << " BB: " << bb.getName());
    }
}

static void get_bbs_with_lib_calls(Function &f, unordered_set<BasicBlock *> &bb_set)
{
    BasicBlock &entry_bb = f.getEntryBlock();
    for (BasicBlock &bb : f) {
        if (!isRecoveredBlock(&bb))
            continue;
        // Discard BBs that has no preds
        if (pred_begin(&bb) == pred_end(&bb) && &bb != &entry_bb) {
            DBG("Discard BB: " << bb.getName());
            continue;
        }

        for (Instruction &i : bb) {
            if (!i.getMetadata("funcname"))
                continue;
            bb_set.insert(&bb);
            DBG("BB with Func Call: " << bb.getName());
            break;
        }
    }
}

static void add_switch_cases(
    map<unsigned, BasicBlock *> &bb_map,
    unordered_map<BasicBlock *, unordered_set<unsigned>> &new_succs)
{

    for (auto &p : new_succs) {
        for (auto *succ : successors(p.first)) {
            p.second.erase(getBlockAddress(succ));
        }

        if (p.second.empty()) {
            continue;
        }

        for (unsigned succ_pc : p.second) {
            BasicBlock *succ_bb = bb_map[succ_pc];
            auto *sw = dyn_cast<SwitchInst>(p.first->getTerminator());
            if (sw == nullptr) {
                continue;
            }
            ConstantInt *addr = ConstantInt::get(Type::getInt32Ty(succ_bb->getContext()), succ_pc);

            // add the case if doesn't exist
            if (sw->findCaseValue(addr) == sw->case_default()) {
                sw->addCase(addr, succ_bb);
            }
        }
    }
}

// NOLINTNEXTLINE
auto FixCFGPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    TraceInfo &ti = am.getResult<TraceInfoAnalysis>(m);
    FunctionInfo fi{ti};

    for (Function &f : m) {
        if (!f.getName().startswith("Func_")) {
            continue;
        }

        unordered_set<BasicBlock *> bb_set;
        get_bbs_with_lib_calls(f, bb_set);

        map<unsigned, BasicBlock *> bb_map;
        get_bbs(f, bb_map);

        unordered_map<unsigned, unordered_set<BasicBlock *>> caller_pc_to_caller_bb;
        find_caller_bbs(f, caller_pc_to_caller_bb, fi);

        list<pair<SwitchInst *, BasicBlock *>> update_list;
        unordered_map<BasicBlock *, unordered_set<BasicBlock *>> jmp_bbs;
        // unordered_map<BasicBlock *, unordered_set<BasicBlock *> >
        // callBBs;

        for (BasicBlock *bb : bb_set) {
            // find caller of this function
            unsigned pc = getBlockAddress(bb);
            auto caller_pc_set = fi.entry_pc_to_caller_pcs[pc];

            // check if each pred is callerBB
            // if it is, then it function called by call inst
            // otherwise, it is called by jmp
            for (auto *p : predecessors(bb)) {
                if (pred_empty(p) && &(f.getEntryBlock()) != p)
                    continue;
                MDNode *md = getBlockMeta(p, "inlined_lib_func");
                if (md) {
                    continue;
                }

                bool found = false;
                for (unsigned caller_pc : caller_pc_set) {
                    for (auto *caller_bb : caller_pc_to_caller_bb[caller_pc])
                        if (caller_bb == p) {
                            // callBBs[B].insert(*P);
                            found = true;
                            break;
                        }
                    if (found)
                        break;
                }
                if (!found && bb_set.find(p) == bb_set.end()) {
                    jmp_bbs[bb].insert(p);
                }
            }
        }

        for (auto &p : jmp_bbs) {
            DBG("BB: " << p.first->getName());
            for (BasicBlock *bb : p.second) {
                DBG("Jump BB: " << bb->getName());
            }
            DBG("-----------------");
        }

        unordered_map<BasicBlock *, unordered_set<unsigned>> new_succs;

        // Add new succs for jmp blocks
        DBG("-----newSuccs-Jump------");
        for (auto &p : jmp_bbs) {
            DBG("BB: " << p.first->getName());
            for (BasicBlock *bb : p.second) {
                // func that called func(p.first) with jmp
                unsigned bb_pc = getBlockAddress(bb);
                DBG("BB_pred_pc: " << utohexstr(bb_pc));
                for (unsigned caller_func_pc : fi.bb_pc_to_entry_pcs[bb_pc]) {
                    DBG("caller func: " << utohexstr(caller_func_pc));
                    for (unsigned caller_of_caller_func : fi.entry_pc_to_caller_pcs[caller_func_pc])
                    {
                        unsigned follow_up = fi.caller_pc_to_follow_up_pc[caller_of_caller_func];
                        DBG("follow_up to be added: " << utohexstr(follow_up));
                        new_succs[p.first].insert(follow_up);
                    }
                }
            }
            DBG("-----------------");
        }

        // Add new succs for call blocks
        DBG("-----newSuccs-Call------");
        for (BasicBlock *bb : bb_set) {
            DBG("BB: " << bb->getName());
            // find caller of this function
            unsigned pc = getBlockAddress(bb);
            auto caller_pc_set = fi.entry_pc_to_caller_pcs[pc];

            // check if each pred is callerBB
            // if it is, then it function called by call inst
            // otherwise, it is called by jmp
            for (auto *p : predecessors(bb)) {
                if (pred_empty(p) && &f.getEntryBlock() != p)
                    continue;
                MDNode *md = getBlockMeta(p, "inlined_lib_func");
                if (md) {
                    continue;
                }

                bool found = false;
                for (unsigned caller_pc : caller_pc_set) {
                    for (auto *caller_bb : caller_pc_to_caller_bb[caller_pc]) {
                        if (caller_bb == p) {
                            unsigned follow_up = fi.caller_pc_to_follow_up_pc[caller_pc];
                            DBG("BB_pred_pc: " << caller_bb->getName());
                            DBG("caller_pc: " << caller_pc
                                              << " followUp: " << utohexstr(follow_up));
                            new_succs[bb].insert(follow_up);
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

        for (auto &p : update_list) {
            ConstantInt *addr = ConstantInt::get(
                Type::getInt32Ty(p.second->getContext()),
                getBlockAddress(p.second));

            p.first->addCase(addr, p.second);
        }

        add_switch_cases(bb_map, new_succs);
    }

    return PreservedAnalyses::none();
}
