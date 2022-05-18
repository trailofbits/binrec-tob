#include "insert_tramp_for_rec_funcs.hpp"
#include "analysis/trace_info_analysis.hpp"
#include "error.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "utils/function_info.hpp"
#include <fstream>
#include <llvm/IR/CFG.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <map>
#include <set>
#include <unordered_set>

#define PASS_NAME "insert_tramp_for_rec_funcs"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;
using namespace std;

static auto get_block_by_name(StringRef name, Function *containing_f) -> BasicBlock *
{
    for (auto &curr_bb : *containing_f) {
        if (curr_bb.getName().equals(name)) {
            return &curr_bb;
        }
    }
    DBG("found no bb called " << name);
    return nullptr;
}

static auto make_enter_tramp(Module &m, BasicBlock *entry_bb) -> BasicBlock *
{
    Function *wrapper = m.getFunction("Func_wrapper");
    PASS_ASSERT(wrapper);
    uint32_t entry_pc = getPc(entry_bb);
    BasicBlock *enter_trampoline =
        BasicBlock::Create(m.getContext(), "enterTrampoline." + entry_bb->getName(), wrapper);
    // assuming call semantics are handled identically in recovered and original
    // code, so only execute version in recovered code
    IRBuilder<> b(enter_trampoline);
    ConstantInt *magic_num = ConstantInt::get(IntegerType::get(m.getContext(), 32), entry_pc);
    DBG("magicNum is " << magic_num->getZExtValue());
    /*Value *args[] = {
        m.getNamedGlobal("R_EAX"),
        m.getNamedGlobal("R_EBX"),
        m.getNamedGlobal("R_ECX"),
        m.getNamedGlobal("R_EDX"),
        m.getNamedGlobal("R_ESI"),
        m.getNamedGlobal("R_EDI"),
        m.getNamedGlobal("R_ESP"),
        m.getNamedGlobal("R_EBP"),
        magicNum,
    };
    doInlineAsm(b,
        "jmp v${:uid}\n\t"
        "movl $8, %eax\n\t"
        "v${:uid}:\n\t"
        "movl %esp, $6\n\t"
        "movl %eax, $0\n\t"
        "movl %ebx, $1\n\t"
        "movl %ecx, $2\n\t"
        "movl %edx, $3\n\t"
        "movl %esi, $4\n\t"
        "movl %edi, $5\n\t"
        "movl %ebp, $7\n\t",
        "*m,*m,*m,*m,*m,*m,*m,*m,i", true, args
    );*/

    /*
        LoadInst *RESPLoad = b.CreateLoad(m.getNamedGlobal("R_ESP"));
        b.CreateStore(RESPLoad, m.getNamedGlobal("prevR_ESP"));
        Value *args[] = {
            m.getNamedGlobal("R_ESP"),
            magicNum,
        };
        doInlineAsm(b,
            "jmp v${:uid}\n\t"
            "movl $1, %eax\n\t"
            "v${:uid}:\n\t"
            "movl %esp, $0\n\t",
            "*m,i", true, args
        );
    */

    // LoadInst *RESPLoad = b.CreateLoad(m.getNamedGlobal("R_ESP"));
    /*    Value *args[] = {
            m.getNamedGlobal("R_ESP"),
            magicNum,
            m.getNamedGlobal("prevR_ESP"),
        };
        doInlineAsm(b,
            "jmp v${:uid}\n\t"
            "movl $1, %eax\n\t"
            "v${:uid}:\n\t"
            "movl $0, %eax\n\t"
            "movl %eax, $2\n\t"
            "movl %esp, $0\n\t",
            "*m,i,*m", true, args
        );
    */
    array<Value *, 2> args = {
        m.getNamedGlobal("R_ESP"),
        magic_num,
    };
    doInlineAsm(
        b,
        "jmp v${:uid}\n\t"
        "movl $1, %eax\n\t"
        "v${:uid}:\n\t"
        "movl $0, %eax\n\t"
        "movl %esp, $0\n\t"
        "movl %eax, %esp\n\t" // swap stacks
        "pusha\n\t"           // save registers to not override over values that was
                              // stored by library code just before callback
        "sub $$8, %esp\n\t",  // Open up some space for parameters of
                              // library calls.
        "*m,i",
        true,
        args);

    b.CreateStore(ConstantInt::getTrue(m.getContext()), m.getNamedGlobal("onUnfallback"));
    // buildWrite(b,"unfallingback at ");
    // buildWriteHex(b,magicNum);
    // buildWriteChar(b, '\n');

    // make the fallback jumptable the predecessor of the trampoline BBs
    BasicBlock *entry_pred = nullptr;
    for (BasicBlock *pred : predecessors(entry_bb)) {
        if (pred_begin(pred) != pred_end(pred)) { // pred has predecessor so it will not get deleted
            entry_pred = pred;
            break;
        }
    }

    auto *pred_switch = dyn_cast<SwitchInst>(entry_pred->getTerminator());
    PASS_ASSERT(pred_switch && "Terminator is not switch");
    ConstantInt *case_tag =
        ConstantInt::get(Type::getInt32Ty(enter_trampoline->getContext()), -entry_pc);
    pred_switch->addCase(case_tag, enter_trampoline);
    // create branch from enterTrampoline to entry
    b.CreateBr(entry_bb);

    DBG(*entry_pred);
    DBG(*enter_trampoline);
    // DBG("created enterTrampoline" << recovFunc->origCallTargetAddress);
    return enter_trampoline;
}

static auto make_exit_tramp(Module &m, BasicBlock *return_bb) -> BasicBlock *
{
    Function *wrapper = m.getFunction("Func_wrapper");
    PASS_ASSERT(wrapper);
    BasicBlock *exit_trampoline = BasicBlock::Create(m.getContext(), "exitTrampoline", wrapper);
    IRBuilder<> exit_b(exit_trampoline);

    exit_b.CreateStore(ConstantInt::getFalse(m.getContext()), m.getNamedGlobal("onUnfallback"));
    // buildWrite(exitB,"exiting unfallingback by jumping to ");
    // buildWriteHex(b,magicNum);
    // buildWriteChar(b, '\n');

    // Pop globals to real regs
    /*Value *args2[] = {
        m.getNamedGlobal("R_EAX"),
        m.getNamedGlobal("R_EBX"),
        m.getNamedGlobal("R_ECX"),
        m.getNamedGlobal("R_EDX"),
        m.getNamedGlobal("R_ESI"),
        m.getNamedGlobal("R_EDI"),
        m.getNamedGlobal("R_ESP"),
        m.getNamedGlobal("R_EBP"),
        m.getNamedGlobal("PC")
    };
    doInlineAsm(exitB,
        "movl $6, %esp\n\t"
        "movl $0, %eax\n\t"
        "movl $1, %ebx\n\t"
        "movl $2, %ecx\n\t"
        "movl $3, %edx\n\t"
        "movl $4, %esi\n\t"
        "movl $5, %edi\n\t"
        "movl $7, %ebp\n\t"
        "pushl $8\n\t"        //PC should have ret address. Push it to jump with
    ret "ret",
        "*m,*m,*m,*m,*m,*m,*m,*m,*m", true, args2
    );*/

    // LoadInst *prevRESP = exitB.CreateLoad(m.getNamedGlobal("prevR_ESP"));
    // exitB.CreateStore(prevRESP, m.getNamedGlobal("R_ESP"));
    // "add %esp, 4\n\t"
    array<Value *, 4> args2 = {
        m.getNamedGlobal("R_EAX"),
        m.getNamedGlobal("R_EDX"),
        m.getNamedGlobal("PC"),
        m.getNamedGlobal("R_ESP"),
    };
    doInlineAsm(
        exit_b,
        "add $$8, %esp\n\t"
        "popa\n\t"
        "movl $0, %eax\n\t"
        "movl $1, %edx\n\t"
        "movl $3, %ecx\n\t"
        "movl %esp, $3\n\t"
        "movl %ecx, %esp\n\t"
        "movl $2, %ecx\n\t"
        "push %ecx\n\t"
        "ret",
        "*m,*m,*m,*m",
        true,
        args2);
    /*
        LoadInst *prevRESP = exitB.CreateLoad(m.getNamedGlobal("prevR_ESP"));
        Value *args2[] = {
            m.getNamedGlobal("R_EAX"),
            m.getNamedGlobal("R_EDX"),
            m.getNamedGlobal("R_ESP"),
            m.getNamedGlobal("PC"),
            prevRESP,
        };
        doInlineAsm(exitB,
            "movl $0, %eax\n\t"
            "movl $1, %edx\n\t"
            "movl $2, %esp\n\t"
            "movl $4, $2\n\t"
            "pushl $3\n\t"        //PC should have ret address. Push it to jump
       with ret "ret",
            "*m,*m,*m,*m,*m", true, args2
        );
    */
    exit_b.CreateUnreachable();

    // patch recovered IR to use have entry from enterTrap and exit to exitTramp
    Instruction *return_bb_term = return_bb->getTerminator();
    IRBuilder<> call_b(return_bb_term);
    Value *on_unfallback = call_b.CreateLoad(m.getNamedGlobal("onUnfallback"));
    // Value *pc = callB.CreateLoad(m.getNamedGlobal("PC"));
    // when we port to 3.8, use this instead of the following 2 lines
    Instruction *then_t = llvm::SplitBlockAndInsertIfThen(on_unfallback, return_bb_term, true);
    // Value *oldAPICmp = callB.CreateICmpEQ(onUnfallback,
    // ConstantInt::getTrue(m.getContext())); TerminatorInst * thenT =
    // llvm::SplitBlockAndInsertIfThen(dyn_cast<Instruction>(oldAPICmp),1,
    // nullptr);

    BranchInst *go_to_exit = BranchInst::Create(exit_trampoline);
    llvm::ReplaceInstWithInst(then_t, go_to_exit);
    return exit_trampoline;
}

/// This function finds BBs(pcs) that are entry BBs and ret BBs. It doesn't
/// match entry to ret. All we know is a BB is entry/return of some callback
/// function.
static void get_cb_entry_and_returns(
    Module &m,
    unordered_set<BasicBlock *> &returns,
    unordered_set<BasicBlock *> &entries,
    const FunctionInfo &fi,
    unordered_set<uint32_t> &callers)
{
    Function *wrapper = m.getFunction("Func_wrapper");
    PASS_ASSERT(wrapper && "Func_wrapper not found");
    Function *trampoline = m.getFunction("helper_stub_trampoline");
    PASS_ASSERT(trampoline && "trampoline not found");

    // iterate over lib functions
    for (auto *u : trampoline->users()) {
        DBG(*u);
        auto *i = cast<Instruction>(u);
        BasicBlock *bb = i->getParent();
        if (bb->getParent() != wrapper)
            continue;
        if (pred_begin(bb) == pred_end(bb))
            continue;

        uint32_t bb_pc = getPc(bb);
        // Check if lib function takes callback func

        auto entry_to_b_bs_it = fi.entry_pc_to_bb_pcs.find(bb_pc);
        if (entry_to_b_bs_it == fi.entry_pc_to_bb_pcs.end()) {
            DBG("No record for the lib function: " << utohexstr(bb_pc));
            continue;
        }

        if (entry_to_b_bs_it->second.size() <= 3)
            continue;

        DBG("lib function pc that calls callback: " << utohexstr(bb_pc));

        auto it = fi.entry_pc_to_return_pcs.find(bb_pc);
        PASS_ASSERT(it != fi.entry_pc_to_return_pcs.end() && "No record for the lib function");

        for (uint32_t ret_pc : it->second) {
            // insert retBB into set
            DBG("callback ret: " << utohexstr(ret_pc));
            BasicBlock *ret_bb = get_block_by_name("BB_" + utohexstr(ret_pc), wrapper);
            PASS_ASSERT(ret_bb && "Couldn't find return BB");
            returns.insert(ret_bb);
        }

        for (auto *succ : successors(bb)) {
            uint32_t succ_pc = getPc(succ);
            // Means this succ(func entry) was not called by this lib func
            if (entry_to_b_bs_it->second.find(succ_pc) == entry_to_b_bs_it->second.end())
                continue;
            bool discard = false;
            for (auto caller : callers) {
                int64_t diff = succ_pc - caller;
                if (diff > 0 && diff < 16)
                    discard = true;
            }
            if (discard)
                continue;
            // if(succPc == 134513843)
            // if(succPc == 134513809)
            //    continue;
            // insert succBB as entry of callback functions
            DBG("callback entry: " << utohexstr(succ_pc));
            entries.insert(succ);
        }
    }

    for (auto *bb : entries) {
        DBG(bb->getName());
    }
    for (auto *bb : returns) {
        DBG(bb->getName());
    }
}

/// this pass expects to be run near the end of the lifting pipeline because it
/// looks for inststart metadata and named values "pc"
// NOLINTNEXTLINE
auto InsertTrampForRecFuncsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    // TODO: find out why local calls in callback IR pass parameter by writing
    // to where R_ESP is pointing instead of pushing it to the stack.
    // TODO: phyton script jumps to beginning of enterTramp. Instead jump to
    // movl %esp R_ESP instruction.

    TraceInfo &ti = am.getResult<TraceInfoAnalysis>(m);
    FunctionInfo fi{ti};

    // Create global bool onUnfallback = false
    auto *g_on_unfallback = cast<GlobalVariable>(
        m.getOrInsertGlobal("onUnfallback", IntegerType::get(m.getContext(), 1)));
    g_on_unfallback->setLinkage(GlobalValue::CommonLinkage);
    ConstantInt *const_int_val = ConstantInt::getFalse(m.getContext());
    g_on_unfallback->setInitializer(const_int_val);
    // set alignment?

    // create a prevR_ESP to store where it was pointing.
    GlobalVariable *r_esp = m.getNamedGlobal("R_ESP");
    PASS_ASSERT(r_esp && "R_ESP global variable doesn't exist");
    auto *prev_r_esp =
        cast<GlobalVariable>(m.getOrInsertGlobal("prevR_ESP", r_esp->getType()->getElementType()));
    prev_r_esp->setInitializer(r_esp->getInitializer());
    prev_r_esp->setLinkage(r_esp->getLinkage());

    // parse input file of functions identified in original binary
    unordered_set<BasicBlock *> entries;
    unordered_set<BasicBlock *> returns;
    unordered_set<uint32_t> callers;
    get_cb_entry_and_returns(m, entries, returns, fi, callers);

    for (BasicBlock *entry_bb : entries) {
        make_enter_tramp(m, entry_bb);
    }
    for (BasicBlock *return_bb : returns) {
        make_exit_tramp(m, return_bb);
    }

    // Write func entry PC values to a file to be processed by patching script.
    ofstream out_file("rfuncs");
    if (!out_file.is_open()) {
        throw binrec::lifting_error{"insert_tramp_for_rec_funcs", "Unable to open file: rfuncs"};
    }
    for (BasicBlock *entry : entries) {
        out_file << getPc(entry) << "\n";
    }
    out_file.close();
    return PreservedAnalyses::none();
}
