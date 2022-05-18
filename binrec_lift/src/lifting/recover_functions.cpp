#include "recover_functions.hpp"
#include "analysis/trace_info_analysis.hpp"
#include "error.hpp"
#include "meta_utils.hpp"
#include "pass_utils.hpp"
#include "pc_utils.hpp"
#include "utils/function_info.hpp"
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <map>
#include <set>
#include <vector>

#define PASS_NAME "recover_functions"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
    // Structure to hold information parsed from the trace for fixing up parents for BBs reached
    // from tail calls
    struct ParsedInfo {
        DenseMap<uint32_t, uint32_t> addr_to_parent_map;
        unordered_map<uint32_t, DenseSet<uint32_t>> cfg;
        DenseSet<uint32_t> entry_addrs;
        unordered_map<uint32_t, DenseSet<uint32_t>> ret_addrs_for_entry_addrs;
        DenseMap<uint32_t, uint32_t> caller_pc_to_follow_up_pc;
        DenseMap<uint32_t, Function *> addr_to_fn_map;
        uint32_t parent = 0;
    };

    /*
     * Locate the address of the main function within the _start() entrypoint. In some
     * captures, _start will call __libc_start_main, passing in the actual address of
     * main() as the first argument. This function iterates over _start() instructions
     * and finds the signature of:
     *
     *     call void @helper_stl_mmu(%struct.CPUX86State* %0, i32 %[var], i32 [const],
     *                               i32 33, i8* null)
     *     store i32 %[var], i32* @R_ESP, align 4
     *
     * This sequence loads a constant into a variable and then stores the variable onto
     * the stack (R_ESP). The constant int *might* be the address of main.
     *
     * The constant int is ignored if it is also referenced in a store instruction
     * to the return_address global, indicating that the address is not the main
     * function.
     *
     * Some captures reference the main address from a register, such as R_EAX, which
     * we wouldn't have the value for at this point, which is why there is a second
     * main locator function: locate_main_addr.
     */
    uint32_t locate_main_addr_in_start(Module &m, BasicBlock *start_bb)
    {
        auto esp = m.getGlobalVariable(
            "R_ESP",
            true); // true is AllowInternalLinkage since registers are made internal
        if (!esp) {
            WARNING("failed to find ESP register; falling back to previous main finder logic");
            return 0;
        }

        auto return_address = m.getGlobalVariable("return_address", true);
        if (!return_address) {
            WARNING(
                "failed to find return_address global; falling back to previous main finder logic");
            return 0;
        }

        Function *helper_stl = m.getFunction("helper_stl_mmu");
        if (!helper_stl) {
            WARNING(
                "failed to find helper_stl_mmu function; falling back to previous main finder logic");
            return 0;
        }

        DBG("attempting to find main function address in _start()");

        Value *main_addr_var = nullptr;
        ConstantInt *main_addr_value = nullptr;
        for (auto iter = start_bb->rbegin(); iter != start_bb->rend(); ++iter) {
            Instruction *inst = &(*iter);
            if (isInstStart(inst) && main_addr_value) {
                // We found the previous instruction and have a constant main function
                // address value. We are done.
                break;
            }

            if (auto store = dyn_cast<StoreInst>(inst)) {
                if (store->getPointerOperand() == esp) {
                    // Store value to ESP. This will be a variable, not a constant, at
                    // this point.
                    DBG("found store to ESP, potentially main address variable: " << *store);
                    main_addr_var = store->getValueOperand();
                } else if (
                    main_addr_value && store->getPointerOperand() == return_address &&
                    store->getValueOperand() == main_addr_value)
                {
                    // We found a constant int that *might* be the address of the main
                    // function. But, we just found it being used as the
                    // return_address. So, the main address we have is not correct,
                    // ignore it.
                    DBG("ignoring return address: " << *main_addr_value);
                    main_addr_var = nullptr;
                    main_addr_value = nullptr;
                }
            } else if (auto call = dyn_cast<CallInst>(inst)) {
                if (main_addr_var && call->getCalledFunction() == helper_stl &&
                    call->getArgOperand(1) == main_addr_var)
                {
                    if (auto value = dyn_cast<ConstantInt>(call->getArgOperand(2))) {
                        // We found a call to helper_stl_mmu, which is storing a
                        // constant int to the variable that we *think* holds the
                        // address of the main function. The constant int might be the
                        // main address.
                        //
                        // We only consider the constant int if it is greater-than 0
                        // and can be stored within a uint32_t.
                        if (!value->isZero() && !value->isNegative() && value->getBitWidth() <= 32)
                        {
                            main_addr_value = value;
                            DBG("found potential main address: " << *main_addr_value);
                        }
                    } else {
                        // We found a store to the variable that we *think* holds the
                        // main function address, but the value isn't a constant int.
                        // So, this variable is not the address of main.
                        DBG("ignore main address variable: " << *main_addr_var);
                        main_addr_var = nullptr;
                    }
                }
            }
        }

        if (main_addr_value) {
            // We found a potential main function address, return it.
            uint32_t main_addr = (uint32_t)main_addr_value->getZExtValue();
            INFO("main function address referenced in _start: " << main_addr);
            return main_addr;
        }

        return 0;
    }


    // NOTE (hbrodin):
    // Previous implementation/versions of BinRec and S2E didn't instrument starting at the
    // entrypoint in the updated S2E we do. Because of this the previous method for identifying
    // "main" doesn't any more. Previously they could rely on multiple "entry-points", first being
    // __libc_csu_init and then, the second time, main. Our updated approach looks like this: First
    // block: _start, it has one call to get the eip, on return from that we enter a block with a
    // call to __libc_start_main, wich later will call main the address of main is passed in
    // register eax. Unfortunately, we don't have any symbols at this point. The ieda is instead to
    // walk the successors to find the third block (having the call to __libc_start_main)
    // and locate the last store to eax, this "should" be the address of main.
    uint32_t locate_main_addr(Function *entrypoint, Module &m)
    {

        auto eax = m.getGlobalVariable(
            "R_EAX",
            true); // true is AllowInternalLinkage since registers are made internal
        if (!eax) {
            LLVM_ERROR(error) << "Failed to get a reference to R_EAX";
            throw binrec::lifting_error{"recover_functions", error};
        }

        std::vector<BasicBlock *> successors;
        auto block = &entrypoint->getEntryBlock();

        // First attempt to get the main address by inspecting the instructions in
        // _start, which eventually calls __libc_start_main with the address of the
        // main function as the first argument.
        if (auto main_addr = locate_main_addr_in_start(m, block)) {
            return main_addr;
        }

        for (size_t i = 0; i < 2; i++) {
            if (!getBlockSuccs(block, successors)) {
                LLVM_ERROR(error) << "Failed to find successors for block:\n" << *block;
                throw binrec::lifting_error{"recover_functions", error};
            }

            if (successors.size() != 1) {
                LLVM_ERROR(error) << "Expected a single successor for potential main block, "
                                  << block->getName() << ": got " << successors.size()
                                  << " successors";
                throw binrec::lifting_error{"recover_functions", error};
            }
            block = successors[0];
            successors.clear();
        }

        // Block is now where we should find the store to eax
        uint32_t lastpc = 0;
        for (auto &ins : *block) {
            if (auto store = dyn_cast<StoreInst>(&ins)) {
                if (store->getPointerOperand() == eax) {
                    if (auto ci = dyn_cast<ConstantInt>(store->getValueOperand())) {
                        lastpc = ci->getZExtValue();
                    }
                }
            }
        }

        return lastpc;
    }


    Function *get_main(Function *entrypoint, Module &m)
    {
        auto mainpc = locate_main_addr(entrypoint, m);
        if (mainpc == 0) {
            LLVM_ERROR(error) << "Failed to located main via entrypoint";
            throw binrec::lifting_error{"recover_functions", error};
        }

        // Return the Func_xxxxxx
        auto main = "Func_" + utohexstr(mainpc);
        return m.getFunction(main);
    }

} // namespace

static auto copy_metadata(Instruction *from, Instruction *to) -> bool
{
    if (!from->hasMetadata())
        return false;

    SmallVector<pair<unsigned, MDNode *>, 8> md_nodes;
    from->getAllMetadataOtherThanDebugLoc(md_nodes);

    for (auto node : md_nodes) {
        unsigned kind = node.first;
        MDNode *md = node.second;
        to->setMetadata(kind, md);
    }

    return true;
}

static auto patch_successors(BasicBlock *bb, multimap<Function *, BasicBlock *> &tb_to_bb) -> bool
{
    Instruction *term = bb->getTerminator();
    if (MDNode *md = term->getMetadata("succs")) {
        vector<Metadata *> operands;

        for (unsigned i = 0, l = md->getNumOperands(); i < l; i++) {
            auto *tb = cast<Function>(cast<ValueAsMetadata>(md->getOperand(i))->getValue());
            auto successor_begin = tb_to_bb.lower_bound(tb);
            if (successor_begin == tb_to_bb.end()) {
                outs() << "Can't find BasicBlock for TB " << tb->getName() << "\n";
                continue;
            }
            BasicBlock *successor = successor_begin->second;
            for (auto successor_end = tb_to_bb.upper_bound(tb); successor_begin != successor_end;
                 ++successor_begin)
            {
                if (successor_begin->second->getParent() == bb->getParent()) {
                    successor = successor_begin->second;
                    break;
                }
                if (successor_begin->second ==
                    &successor_begin->second->getParent()->getEntryBlock()) {
                    successor = successor_begin->second;
                }
            }
            if (successor == nullptr) {
                outs() << "Can't find BB for " << tb->getName() << "\n";
                continue;
            }
            Value *successor_address = &successor->getParent()->getEntryBlock() == successor
                ? static_cast<Value *>(successor->getParent())
                : BlockAddress::get(successor->getParent(), successor);
            operands.push_back(ValueAsMetadata::get(successor_address));
        }

        term->setMetadata("succs", MDNode::get(bb->getContext(), operands));

        return true;
    }

    return false;
}

/// Recursive helper method to explore the CFG of the recorded program to identify BBs which are
/// reached through tail calls.
static void tail_call_helper(uint32_t address, ParsedInfo &info, set<uint32_t> &visited)
{
    // Check if BB already visited
    auto addr_search = visited.find(address);
    if (addr_search != visited.end()) {
        return;
    }
    visited.insert(address);

    auto parent_search = info.addr_to_parent_map.find(address);
    if (parent_search == info.addr_to_parent_map.end()) {
        INFO("Parent for 0x" << utohexstr(address) << " not found.");
        return;
    }
    uint32_t parent_addr = parent_search->second;
    if (parent_addr != info.parent) {
        // Need to fix parent for this BB (function)
        parent_search->second = info.parent;
    }

    auto cfg_search = info.cfg.find(address);
    if (cfg_search == info.cfg.end()) {
        INFO("Address 0x" << utohexstr(address) << " not found in CFG.");
        return;
    }

    // If current address is the return address for the function,
    // dont explore any further
    auto ret_search = info.ret_addrs_for_entry_addrs.find(info.parent);
    if (ret_search != info.ret_addrs_for_entry_addrs.end()) {
        auto self_search = ret_search->second.find(address);
        if (self_search != ret_search->second.end()) {
            return;
        }
    }

    // Explore successors
    for (auto succ : cfg_search->second) {
        // If successor has the same address, dont explore any further
        if (succ == address) {
            continue;
        }
        // If next address is an entry to another function, dont explore any further
        auto entry_search = info.entry_addrs.find(succ);
        if (entry_search != info.entry_addrs.end()) {
            auto fn_search = info.addr_to_fn_map.find(address);
            if (fn_search != info.addr_to_fn_map.end()) {
                auto last_pc = getLastPc(fn_search->second);
                auto follow_up_search = info.caller_pc_to_follow_up_pc.find(last_pc);
                if (follow_up_search != info.caller_pc_to_follow_up_pc.end()) {
                    tail_call_helper(follow_up_search->second, info, visited);
                }
            }
        } else {
            // Recursive call
            tail_call_helper(succ, info, visited);
        }
    }
}

// High level wrapper for fixing BB info for tail calls
static void handle_info_for_tail_calls(
    unordered_map<Function *, DenseSet<Function *>> &tb_map,
    TraceInfo &ti,
    FunctionInfo &fl)
{
    ParsedInfo info;
    info.ret_addrs_for_entry_addrs = fl.get_ret_pcs_for_entry_pcs();
    info.caller_pc_to_follow_up_pc = fl.get_caller_pc_to_follow_up_pc();
    DenseMap<uint32_t, Function *> addr_to_fn_map;

    // CFG as an adjacency list
    for (auto successor : ti.successors) {
        auto pc_search = info.cfg.find(successor.pc);
        if (pc_search == info.cfg.end()) {
            info.cfg[successor.pc] = {};
        }
        info.cfg[successor.pc].insert(successor.successor);
    }

    // For all recorded TBs, collect entry addresses and create mapping
    // from address to Function* instance
    for (const auto &tbs : tb_map) {
        auto entry_addr = getBlockAddress(tbs.first);
        if (entry_addr == 0) {
            continue;
        }
        info.addr_to_parent_map[entry_addr] = entry_addr;
        info.entry_addrs.insert(entry_addr);
        addr_to_fn_map[entry_addr] = tbs.first;
        for (Function *tb : tbs.second) {
            auto node_addr = getBlockAddress(tb);
            if (node_addr == 0) {
                continue;
            }
            info.addr_to_parent_map[node_addr] = entry_addr;
            addr_to_fn_map[node_addr] = tb;
        }
    }
    info.addr_to_fn_map = addr_to_fn_map;

    // Explore CFG and record parent information correctly
    for (auto &tbs : tb_map) {
        set<uint32_t> visited;
        auto entry_addr = getBlockAddress(tbs.first);
        // Dont explore if address is 0 or external symbol
        if (entry_addr == 0 ||
            tbs.first->getEntryBlock().getTerminator()->getMetadata("extern_symbol") != nullptr)
        {
            continue;
        }
        info.parent = entry_addr;
        tail_call_helper(entry_addr, info, visited);
    }

    // Iterate over all BBs and identify which BBs to fix
    map<Function *, vector<pair<Function *, Function *>>> to_fix;
    for (auto &tbs : tb_map) {
        uint32_t parent_addr = getBlockAddress(tbs.first);
        for (Function *tb : tbs.second) {
            uint32_t member_addr = getBlockAddress(tb);

            // Check if block we are analyzing jump to imported symbol
            if (tb->getEntryBlock().getTerminator()->getMetadata("extern_symbol") != nullptr) {
                if (parent_addr != member_addr) {
                    // Need to fix parent for this BB
                    INFO(
                        "[Extern PLT] Remove 0x" << utohexstr(member_addr) << " from 0x"
                                                 << utohexstr(parent_addr));

                    auto tb_search = to_fix.find(tbs.first);
                    if (tb_search == to_fix.end()) {
                        to_fix[tbs.first] = {};
                    }
                    to_fix[tbs.first].push_back(make_pair(tb, tb));
                }
            } else {
                auto member_search = info.addr_to_parent_map.find(member_addr);
                // Check if recorded parent and parent through CFG exploration are the same
                if (parent_addr != member_search->second) {
                    // Need to fix parent for this BB
                    INFO(
                        "Remove 0x" << utohexstr(member_addr) << " from 0x"
                                    << utohexstr(parent_addr));

                    auto actual_parent_search = addr_to_fn_map.find(member_search->second);
                    if (actual_parent_search != addr_to_fn_map.end()) {
                        auto tb_search = to_fix.find(tbs.first);
                        if (tb_search == to_fix.end()) {
                            to_fix[tbs.first] = {};
                        }
                        to_fix[tbs.first].push_back(make_pair(tb, actual_parent_search->second));
                    } else {
                        INFO("Could not fix as parent BB was not found");
                    }
                }
            }
        }
    }

    // Fix identified BB Parents
    for (auto &funcs : to_fix) {
        for (auto &fix_pair : funcs.second) {
            Function *parent = funcs.first;
            Function *member = fix_pair.first;
            Function *actual_parent = fix_pair.second;

            tb_map[parent].erase(member);
            auto new_parent_search = tb_map[actual_parent].find(member);
            if (new_parent_search == tb_map[actual_parent].end()) {
                tb_map[actual_parent].insert(member);
            }
        }
    }
}

// NOLINTNEXTLINE
auto RecoverFunctionsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    TraceInfo &ti = am.getResult<TraceInfoAnalysis>(m);
    FunctionInfo fl{ti};
    unordered_map<uint32_t, set<uint32_t>> cfg;
    DenseSet<uint32_t> visited;

    LLVMContext &context = m.getContext();

    unordered_map<Function *, DenseSet<Function *>> tb_map = fl.get_tbs_by_function_entry(m);
    multimap<Function *, BasicBlock *> tb_to_bb;
    vector<pair<Function *, Function *>> merged_functions;

    // Handle parent information for BBs which are reached through tail calls
    // handle_info_for_tail_calls(tb_map, ti, fl);

    for (const auto &tbs : tb_map) {
        auto *merged_function = Function::Create(
            FunctionType::get(Type::getVoidTy(context), false),
            GlobalValue::InternalLinkage);
        m.getFunctionList().push_back(merged_function);
        merged_functions.push_back(make_pair(merged_function, tbs.first));

        BasicBlock *entry = BasicBlock::Create(context, "", merged_function);

        for (Function *tb : tbs.second) {
            StringRef tb_name = tb->getName();
            SmallString<20> bb_name{"BB"};
            bb_name.append(tb_name.substr(tb_name.rfind('_')));
            BasicBlock *bb = nullptr;
            if (tb == tbs.first) {
                entry->setName(bb_name);
                bb = entry;
            } else {
                bb = BasicBlock::Create(context, bb_name, merged_function);
            }

            if (bb_name == "BB") {
                WARNING(
                    "potentially invalid basic block for function "
                    << (void *)tb << ". See issue #78, "
                    << "https://github.com/trailofbits/binrec-prerelease/issues/78, "
                    << "for more information.");
            }

            Type *arg_type = tb->getArg(0)->getType();
            Value *null_arg = ConstantPointerNull::get(cast<PointerType>(arg_type));
            CallInst *inlining_call = CallInst::Create(tb, null_arg, "", bb);
            ReturnInst::Create(context, bb);

            InlineFunctionInfo inline_info;
            InlineResult inline_result = InlineFunction(*inlining_call, inline_info);
            PASS_ASSERT(inline_result.isSuccess());

            copy_metadata(tb->getEntryBlock().getTerminator(), bb->getTerminator());

            tb_to_bb.emplace(tb, bb);
        }
    }

    // We gather up all merged functions and set their name *after* processing every
    // function. This is related to issue #78, where, if a key in tb_map is also the
    // first value in the value set and is also referenced in another item of tb_map,
    // the function name will be stripped. So, we do this operation afterwards to
    // verify that the function name lives for the duration of the analysis.
    //
    // NOTE: initially this was changed to "item.first->setName()", however this always
    // causes a segfault. So it appears that another part of binrec_lift relies on the
    // function name being erased after merging.
    for (auto item : merged_functions) {
        item.first->takeName(item.second);
    }

    for (auto bb : tb_to_bb) {
        patch_successors(bb.second, tb_to_bb);
    }

    for (auto tb_to_bb_pair = tb_to_bb.begin(), end = tb_to_bb.end(); tb_to_bb_pair != end;
         tb_to_bb_pair = tb_to_bb.upper_bound(tb_to_bb_pair->first))
    {
        Function *f = tb_to_bb_pair->first;
        f->eraseFromParent();
    }

    vector<Function *> entry_points = fl.get_entrypoint_functions(m);
    PASS_ASSERT(!entry_points.empty() && "No entry points, can't recover main");
    auto *wrapper = m.getFunction("Func_wrapper");
    PASS_ASSERT(wrapper);
    BasicBlock *entry_block = BasicBlock::Create(context, "", wrapper);
    // NOTE (hbrodin): Changed how main function is located due to update S2E version
    auto main_func = get_main(entry_points[0], m);
    PASS_ASSERT(main_func && "Failed to locate main-function");
    CallInst::Create(main_func, {}, "", entry_block);
    ReturnInst::Create(context, entry_block);

    return PreservedAnalyses::none();
}
