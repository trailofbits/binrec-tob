#include "pc_jumps.hpp"
#include "analysis/trace_info_analysis.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "pc_utils.hpp"
#include "utils/function_info.hpp"
#include <llvm/Analysis/CFG.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;
using namespace std;

static void insert_prev_pc(Function *wrapper, GlobalVariable *prev_pc)
{
    for (BasicBlock &bb : *wrapper) {
        if (isRecoveredBlock(&bb)) {
            StoreInst *pc_store = getFirstInstStart(&bb);
            new StoreInst(
                pc_store->getValueOperand(),
                prev_pc,
                pc_store->isVolatile(),
                pc_store->getNextNode());
        }
    }
}

static auto create_error_block(Module &m, Function *wrapper) -> BasicBlock *
{
    BasicBlock *err_block = BasicBlock::Create(m.getContext(), "error", wrapper);
    GlobalVariable *pc = m.getNamedGlobal("PC");

    // jump to original .text section
    IRBuilder<> b(err_block);

    switch (fallbackMode) {
    case fallback::NONE:
    case fallback::UNFALLBACK:
        b.CreateUnreachable();
        break;
    case fallback::ERROR1: {
        // create a prevPC global variable
        auto *prev_pc =
            cast<GlobalVariable>(m.getOrInsertGlobal("prevPC", pc->getType()->getElementType()));
        prev_pc->setInitializer(pc->getInitializer());
        prev_pc->setLinkage(pc->getLinkage());
        insert_prev_pc(wrapper, prev_pc);
    }
    case fallback::ERROR:
        b.CreateStore(b.getInt32(-1), m.getNamedGlobal("R_EAX"));
        b.CreateRetVoid();
        break;
    case fallback::BASIC:
    case fallback::EXTENDED:
        array<Value *, 9> args = {
            m.getNamedGlobal("R_EAX"),
            m.getNamedGlobal("R_EBX"),
            m.getNamedGlobal("R_ECX"),
            m.getNamedGlobal("R_EDX"),
            m.getNamedGlobal("R_ESI"),
            m.getNamedGlobal("R_EDI"),
            m.getNamedGlobal("R_ESP"),
            m.getNamedGlobal("R_EBP"),
            b.CreateLoad(pc)};
        doInlineAsm(
            b,
            "movl $6, %esp\n\t"
            "pushl $8\n\t"
            "movl $0, %eax\n\t"
            "movl $1, %ebx\n\t"
            "movl $2, %ecx\n\t"
            "movl $3, %edx\n\t"
            "movl $4, %esi\n\t"
            "movl $5, %edi\n\t"
            "movl $7, %ebp\n\t"
            "ret",
            "*m,*m,*m,*m,*m,*m,*m,*m,r",
            true,
            args);
        b.CreateUnreachable();
        break;
    }

    return err_block;
}

static auto is_possible_jump_target(BasicBlock *bb) -> bool
{
    return isRecoveredBlock(bb) && bb->getName().size() <= 11 &&
        bb != &bb->getParent()->getEntryBlock();
}

static auto create_jump_table(BasicBlock *err_block) -> SwitchInst *
{
    Function *wrapper = err_block->getParent();

    // find error block and count recovered blocks
    unsigned num_cases = 0;
    for (BasicBlock &bb : *wrapper) {
        if (is_possible_jump_target(&bb))
            num_cases++;
    }
    assert(num_cases > 0);

    DBG("creating BB jump table of size " << num_cases);

    // create new jump table block which replaces the error block
    BasicBlock *jump_table_block = BasicBlock::Create(wrapper->getContext(), "jumptable", wrapper);
    err_block->replaceAllUsesWith(jump_table_block);

    IRBuilder<> b(jump_table_block);
    Value *load_pc = b.CreateLoad(wrapper->getParent()->getNamedGlobal("PC"));

    // add a switch statement on the value of @PC that defaults to the old error block
    return b.CreateSwitch(load_pc, err_block, num_cases);
}


static auto find_master_block(BasicBlock *bb) -> BasicBlock *
{
    while (!isRecoveredBlock(bb)) {
        pred_iterator it = pred_begin(bb);

        if (it == pred_end(bb))
            return nullptr;

        bb = *it;
    }

    return bb;
}


// NOLINTNEXTLINE
auto PcJumpsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    TraceInfo &ti = am.getResult<TraceInfoAnalysis>(m);
    FunctionInfo fi{ti};

    GlobalVariable *global_pc = m.getNamedGlobal("PC");

    for (Function &f : m) {
        if (!f.getName().startswith("Func_"))
            continue;

        BasicBlock *err_bb = create_error_block(m, &f);

        for (BasicBlock &bb : f) {
            vector<BasicBlock *> successors;
            BasicBlock *master_block = find_master_block(&bb);
            if (!master_block)
                continue;

            getBlockSuccs(master_block, successors);

            auto *terminator = bb.getTerminator();
            if (!isa<ReturnInst>(terminator) || successors.empty())
                continue;

            bool is_ret_block = ti.functionLog.entryToReturn.find(
                                    make_pair(getPc(&f.getEntryBlock()), getPc(master_block))) !=
                ti.functionLog.entryToReturn.end();
            // FPar: This loop is still in there, because I am not sure whether trace info will find
            // all rets.
            for (BasicBlock *succ : successors) {
                if (succ->getParent() != &f) {
                    assert(is_ret_block);
                    is_ret_block = true;
                    break;
                }
            }

            if (is_ret_block)
                continue;

            auto *load_pc = new LoadInst(global_pc->getValueType(), global_pc, "pc", terminator);
            SwitchInst *sw = SwitchInst::Create(load_pc, err_bb, successors.size(), terminator);

            for (BasicBlock *succ1 : successors) {
                assert(isRecoveredBlock(succ1));
                ConstantInt *addr =
                    ConstantInt::get(Type::getInt32Ty(bb.getContext()), getBlockAddress(succ1));
                sw->addCase(addr, succ1);
            }

            sw->setMetadata("lastpc", terminator->getMetadata("lastpc"));
            sw->setMetadata("inlined_lib_func", terminator->getMetadata("inlined_lib_func"));

            terminator->eraseFromParent();
        }

        for (BasicBlock &bb : f) {
            // Remove metadata here, because it simplifies later passes.
            setBlockSuccs(&bb, {});
        }

        if (fallbackMode == fallback::EXTENDED) {
            SwitchInst *jump_table = create_jump_table(err_bb);

            for (BasicBlock &bb : f) {
                if (is_possible_jump_target(&bb))
                    createJumpTableEntry(jump_table, &bb);
            }
        }
    }
    return PreservedAnalyses::none();
}
