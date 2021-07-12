#include "GlobalEnvToAlloca.h"
#include "IR/Register.h"
#include <llvm/ADT/PostOrderIterator.h>
#include <llvm/ADT/SCCIterator.h>
#include <llvm/Analysis/CallGraph.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
using GvSet = set<GlobalVariable *>;

struct InputOutputRegs {
    map<Function *, GvSet> usedRegs;
    map<Function *, GvSet> inputRegs;
    map<Function *, GvSet> outputRegs;
};

auto getRegs(Module &m) -> GvSet {
    GvSet result;
    for (StringRef name : globalTrivialRegisterNames) {
        GlobalVariable *global = m.getNamedGlobal(name);
        if (global) {
            result.insert(global);
        }
    }
    return result;
}

auto getMustDefs(const Function &f, const map<const BasicBlock *, GvSet> &definingBlocks) -> GvSet {
    map<const BasicBlock *, GvSet> liveRegisters{definingBlocks};
    bool change = true;
    while (change) {
        change = false;
        ReversePostOrderTraversal<const Function *> rpot{&f};
        for (const BasicBlock *bb : rpot) {
            auto predBegin = pred_begin(bb), predEnd = pred_end(bb);
            if (predBegin == predEnd) {
                continue;
            }
            GvSet liveAtBegin = liveRegisters[*predBegin];
            ++predBegin;
            for_each(predBegin, predEnd, [&liveRegisters, &liveAtBegin](const BasicBlock *pred) {
                auto liveFromPred = liveRegisters[pred];
                GvSet intersection;
                set_intersection(liveAtBegin.begin(), liveAtBegin.end(), liveFromPred.begin(), liveFromPred.end(),
                                 inserter(intersection, intersection.begin()));
                liveAtBegin = intersection;
            });

            GvSet &liveRegs = liveRegisters[bb];
            size_t sizeBefore = liveRegs.size();
            copy(liveAtBegin.begin(), liveAtBegin.end(), inserter(liveRegs, liveRegs.begin()));
            change |= liveRegs.size() != sizeBefore;
        }
    }

    GvSet result;
    bool first = true;
    for (const BasicBlock &bb : f) {
        if (!succ_empty(&bb) || isa<UnreachableInst>(bb.getTerminator())) {
            continue;
        }
        auto bbLive = liveRegisters[&bb];
        if (first) {
            result = bbLive;
            first = false;
            continue;
        }
        GvSet intersection;
        set_intersection(result.begin(), result.end(), bbLive.begin(), bbLive.end(),
                         inserter(intersection, intersection.begin()));
        result = intersection;
    }

    return result;
}

auto getInputOutputRegs(Module &m, CallGraph &cg) -> InputOutputRegs {
    GvSet regs = getRegs(m);

    map<Function *, GvSet> fMayUse;
    map<Function *, map<const BasicBlock *, GvSet>> fBbDefs;
    map<Function *, GvSet> fMayDef;

    for (GlobalVariable *reg : regs) {
        for (User *user : reg->users()) {
            if (auto *store = dyn_cast<StoreInst>(user)) {
                BasicBlock *bb = store->getParent();
                Function *f = bb->getParent();
                fBbDefs[f][bb].emplace(reg);
                fMayDef[f].emplace(reg);
                fMayUse[f].emplace(reg);
            } else if (auto *load = dyn_cast<LoadInst>(user)) {
                BasicBlock *bb = load->getParent();
                Function *f = bb->getParent();
                fMayUse[f].emplace(reg);
            }
        }
    }

    map<Function *, GvSet> fMustDef;
    map<Function *, GvSet> fIn;
    map<Function *, GvSet> fOut;

    map<Function *, map<BasicBlock *, GvSet>> fBbLive;

    bool outerChange = true;
    while (outerChange) {
        outerChange = false;

        for (auto it = scc_begin(&cg); !it.isAtEnd(); ++it) {
            const vector<CallGraphNode *> &scc = *it;

            GvSet sccMayDef;

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
                                    const GvSet &cfMustDef = fMustDef[cf];
                                    GvSet &defs = fBbDefs[f][&bb];
                                    copy(cfMustDef.begin(), cfMustDef.end(), inserter(defs, defs.begin()));

                                    const GvSet &cfMayDef = fMayDef[cf];
                                    GvSet &mayDef = fMayDef[f];
                                    copy(cfMayDef.begin(), cfMayDef.end(), inserter(mayDef, mayDef.begin()));
                                }
                            }
                        }
                        const GvSet &mayDef = fMayDef[f];
                        copy(mayDef.begin(), mayDef.end(), inserter(sccMayDef, sccMayDef.begin()));

                        auto &mustDefs = fMustDef[f];
                        size_t oldSize = mustDefs.size();
                        mustDefs = getMustDefs(*f, fBbDefs[f]);
                        change |= mustDefs.size() != oldSize;
                    }
                }
            }

            for (CallGraphNode *cgn : scc) {
                if (Function *f = cgn->getFunction()) {
                    if (f->empty()) {
                        continue;
                    }

                    fMayDef[f] = sccMayDef;
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

                map<BasicBlock *, GvSet> &bbLive = fBbLive[f];

                set<BasicBlock *> workList;
                transform(f->begin(), f->end(), inserter(workList, workList.begin()),
                          [](BasicBlock &bb) { return &bb; });

                while (!workList.empty()) {
                    BasicBlock *bb = *workList.begin();
                    workList.erase(bb);

                    GvSet live;
                    for (BasicBlock *s : successors(bb)) {
                        const GvSet &sLive = fBbLive[f][s];
                        copy(sLive.begin(), sLive.end(), inserter(live, live.begin()));
                    }

                    for_each(bb->rbegin(), bb->rend(), [&](Instruction &i) {
                        if (auto *li = dyn_cast<LoadInst>(&i)) {
                            auto *gv = dyn_cast<GlobalVariable>(li->getPointerOperand());
                            if (regs.find(gv) != regs.end()) {
                                live.insert(gv);
                            }
                        } else if (auto *si = dyn_cast<StoreInst>(&i)) {
                            auto *gv = dyn_cast<GlobalVariable>(si->getPointerOperand());
                            if (regs.find(gv) != regs.end()) {
                                live.erase(gv);
                            }
                        } else if (auto *ci = dyn_cast<CallInst>(&i)) {
                            Function *cf = ci->getCalledFunction();
                            if (cf == nullptr) {
                                return;
                            }
                            GvSet &mustDef = fMustDef[cf];
                            GvSet newLive;
                            set_difference(live.cbegin(), live.cend(), mustDef.cbegin(), mustDef.cend(),
                                           inserter(newLive, newLive.begin()));

                            GvSet out;
                            set_difference(live.cbegin(), live.cend(), newLive.cbegin(), newLive.cend(),
                                           inserter(out, out.begin()));
                            const GvSet &mayDef = fMayDef[cf];
                            set_intersection(mayDef.cbegin(), mayDef.cend(), live.cbegin(), live.cend(),
                                             inserter(out, out.begin()));
                            live = newLive;

                            GvSet &oldOut = fOut[cf];
                            GvSet newOut{out.begin(), out.end()};
                            copy(oldOut.begin(), oldOut.end(), inserter(newOut, newOut.begin()));
                            if (newOut != oldOut) {
                                change = true;
                                oldOut = newOut;
                            }

                            const GvSet &cfIn = fIn[cf];
                            copy(cfIn.cbegin(), cfIn.cend(), inserter(live, live.begin()));
                        } else if (isa<ReturnInst>(i)) {
                            const GvSet &out = fOut[f];
                            copy(out.begin(), out.end(), inserter(live, live.begin()));
                        }
                    });

                    GvSet &oldLive = bbLive[bb];
                    if (live != oldLive) {
                        oldLive = live;
                        copy(pred_begin(bb), pred_end(bb), inserter(workList, workList.begin()));
                    }
                }

                const GvSet &entryLive = bbLive[&f->getEntryBlock()];
                GvSet &in = fIn[f];
                if (entryLive != in) {
                    change = true;
                    in = entryLive;
                }
            }

            if (change) {
                outerChange = true;
            }
        }
    }

    for (auto &f : m) {
        auto &mayUse = fMayUse[&f];
        const GvSet &in = fIn[&f];
        copy(in.begin(), in.end(), inserter(mayUse, mayUse.begin()));
        const GvSet &out = fOut[&f];
        copy(out.begin(), out.end(), inserter(mayUse, mayUse.begin()));
    }

    return {fMayUse, fIn, fOut};
}

struct FunctionRegisterMap {
    Function *oldF{};
    Function *newF{};
    map<Value *, AllocaInst *> localRegs;
    vector<GlobalVariable *> args;
};

auto getArgWeight(const GlobalVariable *var) -> int {
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

auto makeFunctionType(Module &m, const GvSet &inputRegs, const GvSet &outputRegs)
    -> pair<vector<GlobalVariable *>, FunctionType *> {
    GvSet allRegs{inputRegs};
    for (GlobalVariable *reg : outputRegs) {
        allRegs.insert(reg);
    }
    vector<GlobalVariable *> args{allRegs.begin(), allRegs.end()};
    sort(args, [](const GlobalVariable *lhs, const GlobalVariable *rhs) {
        int lhsWeight = getArgWeight(lhs);
        int rhsWeight = getArgWeight(rhs);
        if (lhsWeight == rhsWeight)
            return lhs < rhs;
        return lhsWeight < rhsWeight;
    });

    vector<Type *> signatureArgs;
    for (GlobalVariable *reg : args) {
        bool isRef = outputRegs.find(reg) != outputRegs.end();
        Type *argTy = isRef ? reg->getType() : reg->getValueType();
        signatureArgs.push_back(argTy);
    }
    auto *fTy = FunctionType::get(Type::getVoidTy(m.getContext()), signatureArgs, false);
    return {args, fTy};
}

auto updateSignature(Function &f, const GvSet &usedRegs, const GvSet &inputRegs, const GvSet &outputRegs)
    -> FunctionRegisterMap {
    map<Value *, AllocaInst *> localRegs;
    pair<vector<GlobalVariable *>, FunctionType *> fTy = makeFunctionType(*f.getParent(), inputRegs, outputRegs);
    auto *const newF = Function::Create(fTy.second, f.getLinkage(), f.getAddressSpace(), "", f.getParent());
    newF->takeName(&f);

    unsigned idx = 0;
    for (GlobalVariable *arg : fTy.first) {
        Argument *fArg = newF->getArg(idx);
        StringRef argName = arg->getName().lower();
        fArg->setName("arg_" + argName);
        if (auto *pTy = dyn_cast<PointerType>(fArg->getType())) {
            fArg->addAttr(Attribute::NoCapture);
            fArg->addAttr(Attribute::NoFree);
            fArg->addAttr(Attribute::NoAlias);
            fArg->addAttr(Attribute::get(f.getContext(), Attribute::Dereferenceable,
                                         pTy->getElementType()->getPrimitiveSizeInBits() / 8));
        }
        ++idx;
    }

    auto *entry = BasicBlock::Create(f.getContext(), "entry", newF);
    IRBuilder<> irb{entry};
    for (auto *reg : usedRegs) {
        auto *alloca = irb.CreateAlloca(irb.getInt32Ty(), nullptr, reg->getName().lower());
        localRegs.emplace(reg, alloca);
    }

    idx = 0;
    for (GlobalVariable *arg : fTy.first) {
        if (inputRegs.find(arg) != inputRegs.end()) {
            Argument *fArg = newF->getArg(idx);
            Value *argVal = fArg->getType()->isPointerTy() ? cast<Value>(irb.CreateLoad(fArg)) : fArg;
            irb.CreateStore(argVal, localRegs[arg]);
        }
        ++idx;
    }
    auto *call = irb.CreateCall(&f, {});

    idx = 0;
    for (GlobalVariable *arg : fTy.first) {
        if (outputRegs.find(arg) != outputRegs.end()) {
            Argument *fArg = newF->getArg(idx);
            irb.CreateStore(irb.CreateLoad(localRegs[arg]), fArg);
        }
        ++idx;
    }
    irb.CreateRetVoid();

    InlineFunctionInfo inlineInfo;
    InlineResult inlineResult = InlineFunction(*call, inlineInfo);
    assert(inlineResult.isSuccess());

    for (auto &bb : *newF) {
        for (auto &i : bb) {
            if (auto *load = dyn_cast<LoadInst>(&i)) {
                auto reg = localRegs.find(load->getPointerOperand());
                if (reg != localRegs.end()) {
                    load->setOperand(0, reg->second);
                }
            } else if (auto *store = dyn_cast<StoreInst>(&i)) {
                auto reg = localRegs.find(store->getPointerOperand());
                if (reg != localRegs.end()) {
                    store->setOperand(1, reg->second);
                }
            }
        }
    }

    return {&f, newF, localRegs, fTy.first};
}
} // namespace

// NOLINTNEXTLINE
auto GlobalEnvToAllocaPass::run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
    CallGraph &cg = mam.getResult<CallGraphAnalysis>(m);
    InputOutputRegs inputOutputRegs = getInputOutputRegs(m, cg);

    vector<Function *> fs;
    for (Function &f : m) {
        fs.push_back(&f);
    }

    map<Function *, FunctionRegisterMap> newFunctions;
    for (Function *f : fs) {
        FunctionRegisterMap regMap;
        if (f->getName().startswith("Func_wrapper")) {
            map<Value *, AllocaInst *> localRegs;
            for (GlobalVariable *reg : getRegs(m)) {
                IRBuilder<> irb{&f->getEntryBlock(), f->getEntryBlock().begin()};
                AllocaInst *alloca = irb.CreateAlloca(irb.getInt32Ty(), nullptr, reg->getName().lower());
                localRegs.emplace(reg, alloca);
                irb.CreateStore(irb.CreateLoad(reg), alloca);
                irb.SetInsertPoint(f->getEntryBlock().getTerminator());
                irb.CreateStore(irb.CreateLoad(alloca), reg);
            }
            regMap = FunctionRegisterMap{nullptr, f, move(localRegs), {}};
        } else if (f->getName().startswith("Func_")) {
            regMap = updateSignature(*f, inputOutputRegs.usedRegs[f], inputOutputRegs.inputRegs[f],
                                     inputOutputRegs.outputRegs[f]);
        }
        newFunctions.emplace(regMap.newF, move(regMap));
    }

    vector<CallInst *> callsToRemove;
    for (const auto &[f, calleeRegMap] : newFunctions) {
        if (calleeRegMap.oldF == nullptr) {
            continue;
        }

        for (auto *user : calleeRegMap.oldF->users()) {
            if (auto *call = dyn_cast<CallInst>(user)) {
                callsToRemove.emplace_back(call);

                auto callerRegMapIt = newFunctions.find(call->getParent()->getParent());
                if (callerRegMapIt == newFunctions.end()) {
                    // is call in old or non recovered function
                    continue;
                }

                FunctionRegisterMap &callerRegMap = callerRegMapIt->second;

                IRBuilder<> irb{call};
                SmallVector<Value *, 8> args;
                unsigned idx = 0;
                DenseMap<Value *, AllocaInst *> paramToReg;
                for (GlobalVariable *arg : calleeRegMap.args) {
                    auto localRegIt = callerRegMap.localRegs.find(arg);
                    if (localRegIt == callerRegMap.localRegs.end()) {
                        dbgs() << "Warning: couldn't find local reg " << arg->getName() << " in function "
                               << callerRegMap.newF->getName() << '\n';
                        IRBuilder<> entryBuilder{&call->getParent()->getParent()->getEntryBlock().front()};
                        auto *alloca = entryBuilder.CreateAlloca(irb.getInt32Ty(), nullptr, arg->getName().lower());
                        localRegIt = callerRegMap.localRegs.emplace(arg, alloca).first;
                    }
                    Value *param = irb.CreateLoad(localRegIt->second);
                    args.push_back(param);
                    paramToReg.insert(make_pair(param, localRegIt->second));
                    ++idx;
                }

                CallInst *stacksave = irb.CreateIntrinsic(Intrinsic::stacksave, None, None);

                SmallVector<pair<Value *, AllocaInst *>, 16> restoreRegs;
                for (Argument &arg : f->args()) {
                    if (arg.getType()->isPointerTy()) {
                        Value *&param = args[arg.getArgNo()];
                        auto *argAlloca = irb.CreateAlloca(param->getType());
                        irb.CreateStore(param, argAlloca);
                        restoreRegs.emplace_back(argAlloca, paramToReg[param]);
                        param = argAlloca;
                    }
                }

                irb.CreateCall(f, args);

                for (auto &[from, to] : restoreRegs) {
                    from = irb.CreateLoad(from);
                }

                irb.CreateIntrinsic(Intrinsic::stackrestore, None, {stacksave});

                for (auto [from, to] : restoreRegs) {
                    irb.CreateStore(from, to);
                }
            }
        }
    }

    for (CallInst *call : callsToRemove) {
        assert(call->getNumUses() == 0);
        call->eraseFromParent();
    }

    for (const auto &[f, calleeRegMap] : newFunctions) {
        if (calleeRegMap.oldF) {
            SmallString<64> name = calleeRegMap.oldF->getName();
            calleeRegMap.oldF->eraseFromParent();
        }
    }

    return PreservedAnalyses::none();
}
