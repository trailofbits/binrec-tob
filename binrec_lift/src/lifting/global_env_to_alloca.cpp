#include "global_env_to_alloca.hpp"
#include "error.hpp"
#include "ir/register.hpp"
#include <bitset>
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/ADT/SCCIterator.h>
#include <llvm/ADT/SmallVector.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>

#define PASS_NAME "global_env_to_alloca"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
    using GvSet = set<GlobalVariable *>;
    using RegSet = bitset<12>;

    struct InputOutputRegs {
        InputOutputRegs(
            const vector<GlobalVariable *> &regs,
            const map<Function *, RegSet> &used_regs,
            const map<Function *, RegSet> &input_regs,
            const map<Function *, RegSet> &output_regs) :
                used_regs{bits_to_regs(regs, used_regs)},
                input_regs{bits_to_regs(regs, input_regs)},
                output_regs{bits_to_regs(regs, output_regs)}
        {
        }

        map<Function *, GvSet> used_regs;
        map<Function *, GvSet> input_regs;
        map<Function *, GvSet> output_regs;

    private:
        static auto
        bits_to_regs(const vector<GlobalVariable *> &regs, const map<Function *, RegSet> &bits)
            -> map<Function *, GvSet>
        {
            map<Function *, GvSet> result;
            transform(
                bits.begin(),
                bits.end(),
                inserter(result, result.begin()),
                [&regs](const pair<Function *, RegSet> &value) {
                    GvSet f_regs;
                    for (size_t i = 0; i < regs.size(); ++i) {
                        if (value.second.test(i)) {
                            f_regs.insert(regs[i]);
                        }
                    }
                    return make_pair(value.first, move(f_regs));
                });
            return result;
        }
    };
} // namespace

static auto get_regs(Module &m) -> vector<GlobalVariable *>
{
    vector<GlobalVariable *> result;
    for (StringRef name : Global_Trivial_Register_Names) {
        GlobalVariable *global = m.getNamedGlobal(name);
        if (global) {
            result.push_back(global);
        }
    }
    return result;
}

static auto get_must_defs(const Function &f, const map<const BasicBlock *, RegSet> &defining_blocks)
    -> RegSet
{
    map<const BasicBlock *, RegSet> live_registers{defining_blocks};
    bool change = true;
    while (change) {
        change = false;
        ReversePostOrderTraversal<const Function *> rpot{&f};
        for (const BasicBlock *bb : rpot) {
            if (pred_empty(bb)) {
                continue;
            }
            RegSet live_at_begin;
            live_at_begin.set();
            for (const BasicBlock *pred : predecessors(bb)) {
                live_at_begin &= live_registers[pred];
            }

            RegSet &live_regs = live_registers[bb];
            size_t size_before = live_regs.count();
            live_regs |= live_at_begin;
            change |= live_regs.count() != size_before;
        }
    }

    RegSet result;
    result.set();
    for (const BasicBlock &bb : f) {
        if (!succ_empty(&bb) || isa<UnreachableInst>(bb.getTerminator())) {
            continue;
        }
        PASS_ASSERT(isa<ReturnInst>(bb.getTerminator()));
        result &= live_registers[&bb];
    }

    return result;
}

static auto get_input_output_regs(Module &m, CallGraph &cg) -> InputOutputRegs
{
    vector<GlobalVariable *> regs = get_regs(m);

    map<Function *, RegSet> f_may_use;
    map<Function *, map<const BasicBlock *, RegSet>> f_bb_defs;
    map<Function *, RegSet> f_may_def;

    for (auto it : enumerate(regs)) {
        GlobalVariable *reg = it.value();
        size_t index = it.index();

        for (User *user : reg->users()) {
            if (auto *store = dyn_cast<StoreInst>(user)) {
                BasicBlock *bb = store->getParent();
                Function *f = bb->getParent();
                f_bb_defs[f][bb].set(index);
                f_may_def[f].set(index);
                f_may_use[f].set(index);
            } else if (auto *load = dyn_cast<LoadInst>(user)) {
                BasicBlock *bb = load->getParent();
                Function *f = bb->getParent();
                f_may_use[f].set(index);
            }
        }
    }

    map<Function *, RegSet> f_must_def;
    map<Function *, RegSet> f_in;
    map<Function *, RegSet> f_out;

    map<Function *, map<BasicBlock *, RegSet>> f_bb_live;

    bool outer_change = true;
    while (outer_change) {
        outer_change = false;

        for (auto it = scc_begin(&cg); !it.isAtEnd(); ++it) {
            const vector<CallGraphNode *> &scc = *it;

            RegSet scc_may_def;

            bool change = true;
            while (change) {
                change = false;
                for (CallGraphNode *cgn : scc) {
                    if (Function *f = cgn->getFunction()) {
                        if (f->empty()) {
                            continue;
                        }

                        for (BasicBlock &bb : *f) {
                            for (Instruction &i : bb) {
                                if (auto *call = dyn_cast<CallInst>(&i)) {
                                    Function *cf = call->getCalledFunction();
                                    f_bb_defs[f][&bb] |= f_must_def[cf];
                                    f_may_def[f] |= f_may_def[cf];
                                }
                            }
                        }

                        scc_may_def |= f_may_def[f];

                        RegSet &must_defs = f_must_def[f];
                        size_t old_size = must_defs.count();
                        must_defs = get_must_defs(*f, f_bb_defs[f]);
                        change |= must_defs.count() != old_size;
                    }
                }
            }

            for (CallGraphNode *cgn : scc) {
                if (Function *f = cgn->getFunction()) {
                    if (f->empty()) {
                        continue;
                    }

                    f_may_def[f] = scc_may_def;
                }
            }
        }

        bool change = true;
        while (change) {
            change = false;
            for (CallGraphNode *cgn : ReversePostOrderTraversal{&cg}) {
                Function *f = cgn->getFunction();
                if (!f || f->empty()) {
                    continue;
                }

                map<BasicBlock *, RegSet> &bb_live = f_bb_live[f];

                set<BasicBlock *> work_list;
                transform(
                    f->begin(),
                    f->end(),
                    inserter(work_list, work_list.begin()),
                    [](BasicBlock &bb) { return &bb; });

                while (!work_list.empty()) {
                    BasicBlock *bb = *work_list.begin();
                    work_list.erase(bb);

                    RegSet live;
                    for (BasicBlock *s : successors(bb)) {
                        live |= f_bb_live[f][s];
                    }

                    for_each(bb->rbegin(), bb->rend(), [&](Instruction &i) {
                        if (auto *li = dyn_cast<LoadInst>(&i)) {
                            auto *gv = dyn_cast<GlobalVariable>(li->getPointerOperand());
                            auto pos = find(regs, gv);
                            if (pos != regs.end()) {
                                live.set(pos - regs.begin());
                            }
                        } else if (auto *si = dyn_cast<StoreInst>(&i)) {
                            auto *gv = dyn_cast<GlobalVariable>(si->getPointerOperand());
                            auto pos = find(regs, gv);
                            if (pos != regs.end()) {
                                live.reset(pos - regs.begin());
                            }
                        } else if (auto *ci = dyn_cast<CallInst>(&i)) {
                            Function *cf = ci->getCalledFunction();
                            if (cf == nullptr) {
                                return;
                            }

                            RegSet new_live = live & ~f_must_def[cf];

                            RegSet out = live & ~new_live;
                            RegSet may_def = f_may_def[cf];
                            out |= may_def & live;
                            live = new_live;

                            RegSet &old_out = f_out[cf];
                            RegSet new_out = out | old_out;
                            if (new_out != old_out) {
                                change = true;
                                old_out = new_out;
                            }

                            live |= f_in[cf];
                        } else if (isa<ReturnInst>(i)) {
                            live |= f_out[f];
                        }
                    });

                    RegSet &old_live = bb_live[bb];
                    if (live != old_live) {
                        old_live = live;
                        copy(pred_begin(bb), pred_end(bb), inserter(work_list, work_list.begin()));
                    }
                }

                RegSet entry_live = bb_live[&f->getEntryBlock()];
                RegSet &in = f_in[f];
                if (entry_live != in) {
                    change = true;
                    in = entry_live;
                }
            }

            if (change) {
                outer_change = true;
            }
        }
    }

    for (auto &f : m) {
        auto &may_use = f_may_use[&f];
        may_use |= f_in[&f];
        may_use |= f_out[&f];
    }

    return InputOutputRegs{regs, f_may_use, f_in, f_out};
}

namespace {
    struct FunctionRegisterMap {
        Function *old_f{};
        Function *new_f{};
        map<Value *, AllocaInst *> local_regs;
        SmallVector<GlobalVariable *, 8> args;
        SmallVector<GlobalVariable *, 8> ret_vals;
    };
} // namespace

static auto global_reg_weight(const GlobalVariable *var) -> int
{
    if (!var) {
        return 0;
    }
    StringRef name = var->getName();
    if (name == "R_ESP") {
        return -3;
    }
    if (name == "R_EAX") {
        return -2;
    }
    if (name == "R_EDX") {
        return -1;
    }
    return 0;
}

static auto sort_regs(const GlobalVariable *lhs, const GlobalVariable *rhs) -> bool
{
    int lhs_weight = global_reg_weight(lhs);
    int rhs_weight = global_reg_weight(rhs);
    if (lhs_weight == rhs_weight)
        return lhs->getName() < rhs->getName();
    return lhs_weight < rhs_weight;
}

namespace {
    struct Signature {
    public:
        FunctionType *f_ty;
        SmallVector<GlobalVariable *, 8> args;
        SmallVector<GlobalVariable *, 8> ret_vals;
    };
} // namespace

static auto make_return_type(LLVMContext &ctx, unsigned ret_count) -> StructType *
{
    SmallVector<Type *, 8> types{ret_count, IntegerType::getInt32Ty(ctx)};
    auto *ty = StructType::get(ctx, types);
    return ty;
}

static auto make_function_type(Module &m, const GvSet &input_regs, const GvSet &output_regs)
    -> Signature
{
    SmallVector<GlobalVariable *, 8> args{input_regs.begin(), input_regs.end()};
    sort(args, sort_regs);

    SmallVector<GlobalVariable *, 8> ret_vals{output_regs.begin(), output_regs.end()};
    sort(ret_vals, sort_regs);
    StructType *ret_struct = make_return_type(m.getContext(), ret_vals.size());

    vector<Type *> signature{args.size(), Type::getInt32Ty(m.getContext())};
    auto *f_ty = FunctionType::get(ret_struct, signature, false);
    return {f_ty, move(args), move(ret_vals)};
}

static auto update_signature(
    Function &f,
    const GvSet &used_regs,
    const GvSet &input_regs,
    const GvSet &output_regs) -> FunctionRegisterMap
{
    map<Value *, AllocaInst *> local_regs;
    Signature sig = make_function_type(*f.getParent(), input_regs, output_regs);
    auto *new_f =
        Function::Create(sig.f_ty, f.getLinkage(), f.getAddressSpace(), "", f.getParent());
    new_f->takeName(&f);

    for (auto g_reg : enumerate(sig.args)) {
        Argument *f_arg = new_f->getArg(g_reg.index());
        f_arg->setName("arg_" + g_reg.value()->getName().drop_front(2).lower());
    }

    auto *entry = BasicBlock::Create(f.getContext(), "", new_f);
    IRBuilder<> irb{entry};
    for (auto *g_reg : used_regs) {
        auto *alloca = irb.CreateAlloca(irb.getInt32Ty(), nullptr, g_reg->getName().lower());
        local_regs.emplace(g_reg, alloca);
    }

    for (auto g_reg : enumerate(sig.args)) {
        if (input_regs.find(g_reg.value()) != input_regs.end()) {
            Argument *f_arg = new_f->getArg(g_reg.index());
            irb.CreateStore(f_arg, local_regs[g_reg.value()]);
        }
    }
    auto *call = irb.CreateCall(&f, {});

    vector<Value *> ret_vals;
    for (auto ret_val : enumerate(sig.ret_vals)) {
        auto *val = irb.CreateLoad(local_regs[ret_val.value()]);
        ret_vals.push_back(val);
    }
    irb.CreateAggregateRet(ret_vals.data(), ret_vals.size());

    InlineFunctionInfo inline_info;
    InlineResult inline_result = InlineFunction(*call, inline_info);
    PASS_ASSERT(inline_result.isSuccess());

    for (auto &bb : *new_f) {
        for (auto &i : bb) {
            if (auto *load = dyn_cast<LoadInst>(&i)) {
                auto reg = local_regs.find(load->getPointerOperand());
                if (reg != local_regs.end()) {
                    load->setOperand(0, reg->second);
                }
            } else if (auto *store = dyn_cast<StoreInst>(&i)) {
                auto reg = local_regs.find(store->getPointerOperand());
                if (reg != local_regs.end()) {
                    store->setOperand(1, reg->second);
                }
            }
        }
    }

    SmallDenseMap<GlobalVariable *, unsigned> gv_to_ret_index;
    for (auto g_reg : enumerate(sig.ret_vals)) {
        gv_to_ret_index.insert({g_reg.value(), g_reg.index()});
    }
    SmallVector<Metadata *, 8> ret_mapping;
    for (auto *arg : sig.args) {
        auto ret_index = gv_to_ret_index.find(arg);
        if (ret_index != gv_to_ret_index.end()) {
            ret_mapping.push_back(ValueAsMetadata::getConstant(
                ConstantInt::get(Type::getInt32Ty(f.getContext()), ret_index->second)));
        } else {
            ret_mapping.push_back(ValueAsMetadata::getConstant(
                ConstantInt::get(Type::getInt32Ty(f.getContext()), 0)));
        }
    }
    new_f->setMetadata("retregs", MDNode::get(f.getContext(), ret_mapping));

    return {&f, new_f, local_regs, sig.args, sig.ret_vals};
}

// NOLINTNEXTLINE
auto GlobalEnvToAllocaPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    CallGraph &cg = am.getResult<CallGraphAnalysis>(m);
    InputOutputRegs input_output_regs = get_input_output_regs(m, cg);

    vector<Function *> fs;
    for (Function &f : m) {
        fs.push_back(&f);
    }

    map<Function *, FunctionRegisterMap> new_functions;
    for (Function *f : fs) {
        FunctionRegisterMap reg_map;
        if (f->getName().startswith("Func_wrapper")) {
            map<Value *, AllocaInst *> local_regs;
            for (GlobalVariable *reg : get_regs(m)) {
                IRBuilder<> irb{&f->getEntryBlock(), f->getEntryBlock().begin()};
                AllocaInst *alloca =
                    irb.CreateAlloca(irb.getInt32Ty(), nullptr, reg->getName().lower());
                local_regs.emplace(reg, alloca);
                irb.CreateStore(irb.CreateLoad(reg), alloca);
                irb.SetInsertPoint(f->getEntryBlock().getTerminator());
                irb.CreateStore(irb.CreateLoad(alloca), reg);
            }
            reg_map = FunctionRegisterMap{nullptr, f, move(local_regs), {}};
        } else if (f->getName().startswith("Func_")) {
            reg_map = update_signature(
                *f,
                input_output_regs.used_regs[f],
                input_output_regs.input_regs[f],
                input_output_regs.output_regs[f]);
        }
        new_functions.emplace(reg_map.new_f, move(reg_map));
    }

    vector<CallInst *> calls_to_remove;
    for (const auto &[f, callee_map] : new_functions) {
        if (callee_map.old_f == nullptr) {
            continue;
        }

        for (auto *user : callee_map.old_f->users()) {
            if (auto *call = dyn_cast<CallInst>(user)) {
                calls_to_remove.emplace_back(call);

                auto caller_map_it = new_functions.find(call->getParent()->getParent());
                if (caller_map_it == new_functions.end()) {
                    // is call in old or non recovered function
                    continue;
                }
                FunctionRegisterMap &caller_map = caller_map_it->second;

                SmallVector<Value *, 8> args;

                IRBuilder<> entry_irb{&call->getParent()->getParent()->getEntryBlock().front()};

                IRBuilder<> irb{call};
                for (GlobalVariable *arg : callee_map.args) {
                    auto local_reg_it = caller_map.local_regs.find(arg);
                    if (local_reg_it == caller_map.local_regs.end()) {
                        dbgs() << "Warning: couldn't find local reg " << arg->getName()
                               << " in function " << caller_map.new_f->getName() << '\n';
                        auto *alloca = entry_irb.CreateAlloca(
                            irb.getInt32Ty(),
                            nullptr,
                            arg->getName().lower());
                        local_reg_it = caller_map.local_regs.emplace(arg, alloca).first;
                    }
                    Value *param = irb.CreateLoad(local_reg_it->second);
                    args.push_back(param);
                }

                Value *ret_regs = irb.CreateCall(f, args);

                for (auto ret_val : enumerate(callee_map.ret_vals)) {
                    irb.CreateStore(
                        irb.CreateExtractValue(ret_regs, ret_val.index()),
                        caller_map.local_regs[ret_val.value()]);
                }
            }
        }
    }

    for (CallInst *call : calls_to_remove) {
        PASS_ASSERT(call->getNumUses() == 0);
        call->eraseFromParent();
    }

    for (const auto &[f, callee_map] : new_functions) {
        if (callee_map.old_f) {
            SmallString<64> name = callee_map.old_f->getName();
            callee_map.old_f->eraseFromParent();
        }
    }

    return PreservedAnalyses::none();
}
