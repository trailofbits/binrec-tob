#include "insert_calls.hpp"
#include "analysis/trace_info_analysis.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "pc_utils.hpp"
#include "utils/function_info.hpp"
#include <map>
#include <set>
#include <vector>

using namespace binrec;
using namespace llvm;

/// This function deals with the slight mismatch of "recovered blocks" and the
/// fact that a recovered block can consist of multiple successors. When trying
/// to change the store to the last PC of a recovered block, we have to find the
/// last block the make up the recovered block. An example is a block that
/// contains a `setz` instruction. Such instructions are translated into a
/// branch followed by a phi. Look for "Func_XXXXXXX.exit" blocks.
static auto find_exit_block(BasicBlock *bb) -> BasicBlock *
{
    BasicBlock *exit_block = bb;

    while (succ_begin(exit_block) != succ_end(exit_block)) {
        exit_block = *succ_begin(exit_block);
    }

    return exit_block;
}

// NOLINTNEXTLINE
auto InsertCallsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    TraceInfo &ti = am.getResult<TraceInfoAnalysis>(m);
    FunctionInfo fi{ti};

    std::map<Function *, std::set<BasicBlock *>> all_ret_blocks =
        fi.get_ret_bbs_by_merged_function(m);

    for (Function &f : m) {

        if (!f.getName().startswith("Func_") ||
            f.getEntryBlock().getTerminator()->getMetadata("extern_symbol") != nullptr)
        {
            continue;
        }

        std::set<BasicBlock *> &ret_blocks = all_ret_blocks[&f];

        for (BasicBlock &bb : f) {
            if (ret_blocks.find(&bb) != ret_blocks.end()) {
                continue;
            }

            std::vector<BasicBlock *> successors;
            if (!getBlockSuccs(&bb, successors) || successors.empty()) {
                continue;
            }

            // Q: Is it safe to assume that the entry block being a successor means there is
            // a recursive call, rather than a branch? A: Probably yes, because there should
            // not be a branch from within the function to its prologue?
            if (successors[0]->getParent() != &f || successors[0] == &f.getEntryBlock()) {
                BasicBlock *exit_block = find_exit_block(&bb);

                BasicBlock *call_follow_up_block = nullptr;

                uint32_t call_pc = getLastPc(&bb);
                auto call_follow_up_pc = fi.caller_pc_to_follow_up_pc.find(call_pc);
                if (call_follow_up_pc != fi.caller_pc_to_follow_up_pc.end()) {
                    // When the call does not return (e.g. call to exit()) there is no
                    // follow up block.
                    std::string call_follow_up_block_name =
                        std::string("BB_") + utohexstr(call_follow_up_pc->second);
                    for (BasicBlock &bb2 : f) {
                        if (bb2.getName() == call_follow_up_block_name) {
                            call_follow_up_block = &bb2;
                        }
                    }
                    if (call_follow_up_block == nullptr) {
                        errs() << "Can't find call follow up block " << call_follow_up_block_name
                               << " of block " << bb.getName() << " in function " << f.getName()
                               << ".\n";
                        assert(call_follow_up_block);
                    }
                }

                if (successors.size() == 1) {
                    // Direct call
                    StoreInst *target_pc_store = findLastPCStore(*exit_block);
                    if (target_pc_store == nullptr) {
                        errs() << "Can't find PC store on callsite in block " << bb.getName()
                               << ".\n";
                        continue;
                    }
                    target_pc_store->eraseFromParent();

                    CallInst::Create(
                        successors[0]->getParent(),
                        {},
                        "",
                        exit_block->getTerminator());

                    StoreInst *call_inst_pc_store = getLastInstStart(exit_block);
                    bb.getTerminator()->setMetadata(
                        "lastpc",
                        MDNode::get(
                            bb.getContext(),
                            ValueAsMetadata::get(call_inst_pc_store->getValueOperand())));

                    if (call_follow_up_block == nullptr) {
                        // chinmay_dd: Ideally if a callFollowUpBlock is not found,
                        // inserting an UnreachableInst here would make sense. However for
                        // tail calls this causes the removal of the return instruction
                        // in the exitBlock.
                        setBlockSuccs(&bb, {});
                    } else {
                        new StoreInst(
                            getFirstInstStart(call_follow_up_block)->getValueOperand(),
                            m.getNamedGlobal("PC"),
                            exit_block->getTerminator());
                        std::vector<BasicBlock *> meta_succs{call_follow_up_block};
                        setBlockSuccs(&bb, meta_succs);
                    }
                } else {
                    unsigned last_pc = getLastPc(&bb);

                    // Create join block after calls
                    auto *terminator = cast<ReturnInst>(exit_block->getTerminator());
                    BasicBlock *join_block =
                        BasicBlock::Create(m.getContext(), bb.getName() + "_join", &f);
                    join_block->moveAfter(exit_block);

                    if (call_follow_up_block == nullptr) {
                        assert(
                            false &&
                            "This is not implemented yet. Also, is metadata set correctly with indirect calls?");
                    } else {
                        ReturnInst::Create(
                            bb.getContext(),
                            terminator->getReturnValue(),
                            join_block);
                        new StoreInst(
                            getFirstInstStart(call_follow_up_block)->getValueOperand(),
                            m.getNamedGlobal("PC"),
                            join_block->getTerminator());
                        std::vector<BasicBlock *> meta_succs{call_follow_up_block};
                        setBlockSuccs(join_block, meta_succs);
                        join_block->getTerminator()->setMetadata(
                            "lastpc",
                            MDNode::get(
                                bb.getContext(),
                                ValueAsMetadata::get(ConstantInt::get(
                                    IntegerType::getInt32Ty(bb.getContext()),
                                    last_pc))));
                    }

                    // Call of function pointer.
                    auto *pc_global = m.getNamedGlobal("PC");
                    auto *target_pc_load = new LoadInst(
                        pc_global->getValueType(),
                        pc_global,
                        "",
                        exit_block->getTerminator());
                    BasicBlock *err_block =
                        BasicBlock::Create(m.getContext(), bb.getName() + "_error", &f);
                    err_block->moveAfter(join_block);
                    new UnreachableInst(bb.getContext(), err_block);
                    SwitchInst *call_switch = SwitchInst::Create(
                        target_pc_load,
                        err_block,
                        successors.size(),
                        exit_block);

                    for (BasicBlock *successor : successors) {
                        assert(
                            (successor->getParent() != &f || successor == &f.getEntryBlock()) &&
                            "Successor is basic block in own function instead of "
                            "a function for function pointer call.");
                        Function *callee = successor->getParent();
                        BasicBlock *call_block = BasicBlock::Create(
                            m.getContext(),
                            std::string("call_") + callee->getName() + "_from_" + bb.getName(),
                            &f);
                        call_block->moveAfter(exit_block);
                        BranchInst::Create(join_block, call_block);

                        ConstantInt *successor_pc = ConstantInt::get(
                            Type::getInt32Ty(bb.getContext()),
                            getBlockAddress(successor));
                        call_switch->addCase(successor_pc, call_block);

                        CallInst::Create(callee, {}, "", call_block->getTerminator());
                    }

                    terminator->eraseFromParent();
                }
            }
        }
    }
    return PreservedAnalyses::none();
}
