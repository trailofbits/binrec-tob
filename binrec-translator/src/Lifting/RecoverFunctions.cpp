#include "Analysis/TraceInfoAnalysis.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include "Utils/FunctionInfo.h"
#include <iterator>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <llvm/Transforms/Utils/Cloning.h>
#include <map>
#include <set>
#include <sstream>
#include <vector>

using namespace binrec;
using namespace llvm;

namespace {
// Structure to hold information parsed from the trace for fixing up parents
// for BBs reached from tail calls
typedef struct {
    std::map<uint32_t, uint32_t> AddrToParentMap;
    std::map<uint32_t, std::set<uint32_t>> CFG;
    std::set<uint32_t> EntryAddrs;
    std::map<uint32_t, std::set<uint32_t>> RetAddrsForEntryAddrs;
    uint32_t Parent;
} ParsedInfo;

auto copyMetadata(Instruction *From, Instruction *To) -> bool {
    if (!From->hasMetadata())
        return false;

    SmallVector<std::pair<unsigned, MDNode *>, 8> MDNodes;
    From->getAllMetadataOtherThanDebugLoc(MDNodes);

    for (auto node : MDNodes) {
        unsigned Kind = node.first;
        MDNode *MD = node.second;
        To->setMetadata(Kind, MD);
    }

    return true;
}

auto patchSuccessors(BasicBlock *BB, std::multimap<Function *, BasicBlock *> &TBToBB) -> bool {
    Instruction *Term = BB->getTerminator();
    if (MDNode *MD = Term->getMetadata("succs")) {
        std::vector<Metadata *> Operands = {};

        for (unsigned I = 0, L = MD->getNumOperands(); I < L; I++) {
            auto *TB = cast<Function>(cast<ValueAsMetadata>(MD->getOperand(I))->getValue());
            auto SuccessorBegin = TBToBB.lower_bound(TB);
            if (SuccessorBegin == TBToBB.end()) {
                outs() << "Can't find BasicBlock for TB " << TB->getName() << "\n";
                continue;
            }
            BasicBlock *Successor = SuccessorBegin->second;
            for (auto SuccessorEnd = TBToBB.upper_bound(TB); SuccessorBegin != SuccessorEnd; ++SuccessorBegin) {
                if (SuccessorBegin->second->getParent() == BB->getParent()) {
                    Successor = SuccessorBegin->second;
                    break;
                }
                if (SuccessorBegin->second == &SuccessorBegin->second->getParent()->getEntryBlock()) {
                    Successor = SuccessorBegin->second;
                }
            }
            if (Successor == nullptr) {
                outs() << "Can't find BB for " << TB->getName() << "\n";
                continue;
            }
            Value *SuccessorAddress = &Successor->getParent()->getEntryBlock() == Successor
                                          ? static_cast<Value *>(Successor->getParent())
                                          : BlockAddress::get(Successor->getParent(), Successor);
            Operands.push_back(ValueAsMetadata::get(SuccessorAddress));
        }

        Term->setMetadata("succs", MDNode::get(BB->getContext(), Operands));

        return true;
    }

    return false;
}

// Recursive helper method to explore the CFG of the recorded program to identify
// BBs which are reached through tail calls.
void tailCallHelper(uint32_t address, ParsedInfo &info, std::set<uint32_t> &visited) {
    // Check if BB already visited
    auto addrSearch = visited.find(address);
    if (addrSearch != visited.end()) {
        return;
    }
    visited.insert(address);

    auto parentSearch = info.AddrToParentMap.find(address);
    if (parentSearch == info.AddrToParentMap.end()) {
        INFO("Parent for 0x" << intToHex(address) << " not found.");
        return;
    }
    uint32_t parentAddr = parentSearch->second;
    if (parentAddr != info.Parent) {
        // Need to fix parent for this BB (function)
        parentSearch->second = info.Parent;
    }

    auto cfgSearch = info.CFG.find(address);
    if (cfgSearch == info.CFG.end()) {
        INFO("Address 0x" << intToHex(address) << " not found in CFG.");
        return;
    }

    // If current address is the return address for the function,
    // dont explore any further
    auto retSearch = info.RetAddrsForEntryAddrs.find(info.Parent);
    if (retSearch != info.RetAddrsForEntryAddrs.end()) {
        auto selfSearch = retSearch->second.find(address);
        if (selfSearch != retSearch->second.end()) {
            return;
        }
    }

    // Explore successors
    for (auto succ : cfgSearch->second) {
        // If successor has the same address, dont explore any further
        if (succ == address) {
            continue;
        }
        // If next address is an entry to another function, dont explore any further
        auto entrySearch = info.EntryAddrs.find(succ);
        if (entrySearch != info.EntryAddrs.end()) {
            continue;
        }
        // Recursive call
        tailCallHelper(succ, info, visited);
    }
}

// High level wrapper for fixing BB info for tail calls
void handleInfoForTailCalls(std::map<Function *, std::set<Function *>> &TBMap, TraceInfo &ti, FunctionInfo &fl) {
    ParsedInfo info;
    info.AddrToParentMap = {};
    info.CFG = {};
    info.EntryAddrs = {};
    info.RetAddrsForEntryAddrs = fl.getRetPcsForEntryPcs();
    std::map<uint32_t, Function *> AddrToFnMap;

    // CFG as an adjacency list
    for (auto successor : ti.successors) {
        auto pcSearch = info.CFG.find(successor.pc);
        if (pcSearch == info.CFG.end()) {
            info.CFG[successor.pc] = {};
        }
        info.CFG[successor.pc].insert(successor.successor);
    }

    // For all recorded TBs, collect entry addresses and create mapping
    // from address to Function* instance
    for (const auto &TBs : TBMap) {
        auto entryAddr = getBlockAddress(TBs.first);
        if (entryAddr == 0) {
            continue;
        }
        info.AddrToParentMap[entryAddr] = entryAddr;
        info.EntryAddrs.insert(entryAddr);
        AddrToFnMap[entryAddr] = TBs.first;
        for (Function *TB : TBs.second) {
            auto nodeAddr = getBlockAddress(TB);
            if (nodeAddr == 0) {
                continue;
            }
            info.AddrToParentMap[nodeAddr] = entryAddr;
            AddrToFnMap[nodeAddr] = TB;
        }
    }

    // Explore CFG and record parent information correctly
    std::set<uint32_t> visited;
    for (auto &TBs : TBMap) {
        visited = {};
        auto entryAddr = getBlockAddress(TBs.first);
        // Dont explore if address is 0 or external symbol
        if (entryAddr == 0 || TBs.first->getEntryBlock().getTerminator()->getMetadata("extern_symbol") != nullptr) {
            continue;
        }
        info.Parent = entryAddr;
        tailCallHelper(entryAddr, info, visited);
    }

    // Iterate over all BBs and identify which BBs to fix
    std::map<Function *, std::vector<std::pair<Function *, Function *>>> toFix = {};
    for (auto &TBs : TBMap) {
        uint32_t parentAddr = getBlockAddress(TBs.first);
        for (Function *TB : TBs.second) {
            uint32_t memberAddr = getBlockAddress(TB);

            // Check if block we are analyzing jump to imported symbol
            if (TB->getEntryBlock().getTerminator()->getMetadata("extern_symbol") != nullptr) {
                if (parentAddr != memberAddr) {
                    // Need to fix parent for this BB
                    INFO("[Extern PLT] Remove 0x" << intToHex(memberAddr) << " from 0x" << intToHex(parentAddr));

                    auto tbSearch = toFix.find(TBs.first);
                    if (tbSearch == toFix.end()) {
                        toFix[TBs.first] = {};
                    }
                    toFix[TBs.first].push_back(std::make_pair(TB, TB));
                }
            } else {
                auto memberSearch = info.AddrToParentMap.find(memberAddr);
                // Check if recorded parent and parent through CFG exploration are the same
                if (parentAddr != memberSearch->second) {
                    // Need to fix parent for this BB
                    INFO("Remove 0x" << intToHex(memberAddr) << " from 0x" << intToHex(parentAddr));

                    auto actualParentSearch = AddrToFnMap.find(memberSearch->second);
                    if (actualParentSearch != AddrToFnMap.end()) {
                        auto tbSearch = toFix.find(TBs.first);
                        if (tbSearch == toFix.end()) {
                            toFix[TBs.first] = {};
                        }
                        toFix[TBs.first].push_back(std::make_pair(TB, actualParentSearch->second));
                    } else {
                        INFO("Could not fix as parent BB was not found");
                    }
                }
            }
        }
    }

    // Fix identified BB Parents
    for (auto &Funcs : toFix) {
        for (auto &FixPair : Funcs.second) {
            Function *Parent = Funcs.first;
            Function *Member = FixPair.first;
            Function *ActualParent = FixPair.second;

            TBMap[Parent].erase(Member);
            auto newParentSearch = TBMap[ActualParent].find(Member);
            if (newParentSearch == TBMap[ActualParent].end()) {
                TBMap[ActualParent].insert(Member);
            }
        }
    }
}

/// Merge translations blocks that belong to the same function.
class RecoverFunctionsPass : public PassInfoMixin<RecoverFunctionsPass> {
public:
    // NOLINTNEXTLINE
    auto run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
        TraceInfo &ti = mam.getResult<TraceInfoAnalysis>(m);
        FunctionInfo fl{ti};
        std::map<uint32_t, std::set<uint32_t>> CFG;
        std::set<uint32_t> visited;

        LLVMContext &context = m.getContext();

        std::map<Function *, std::set<Function *>> TBMap = fl.getTBsByFunctionEntry(m);
        std::multimap<Function *, BasicBlock *> TBToBB;

        // Handle parent information for BBs which are reached through tail calls
        handleInfoForTailCalls(TBMap, ti, fl);

        for (const auto &TBs : TBMap) {
            auto *MergedFunction = cast<Function>(
                m.getOrInsertFunction("", FunctionType::get(Type::getVoidTy(context), false)).getCallee());

            // Create an entry block to have all allocas of inlining inserted here.
            assert(MergedFunction->empty());
            BasicBlock *Entry = BasicBlock::Create(context, "entry", MergedFunction);

            for (Function *TB : TBs.second) {
                std::string TBName = TB->getName().str();
                std::string BBName = "BB" + TBName.substr(TBName.rfind('_'));
                BasicBlock *BB = nullptr;
                if (TB == TBs.first) {
                    Entry->setName(BBName);
                    BB = Entry;
                } else {
                    BB = BasicBlock::Create(context, BBName, MergedFunction);
                }

                Type *ArgType = TB->getArg(0)->getType();
                Value *NullArg = ConstantPointerNull::get(cast<PointerType>(ArgType));
                CallInst *InliningCall = CallInst::Create(TB, NullArg, "", BB);
                ReturnInst::Create(context, BB);

                InlineFunctionInfo InlineInfo;
                InlineResult inlineResult = InlineFunction(*InliningCall, InlineInfo);
                assert(inlineResult.isSuccess());

                copyMetadata(TB->getEntryBlock().getTerminator(), BB->getTerminator());

                TBToBB.emplace(TB, BB);
            }

            MergedFunction->takeName(TBs.first);
        }

        for (const auto &TBToBBPair : TBToBB) {
            BasicBlock *BB = TBToBBPair.second;
            patchSuccessors(BB, TBToBB);
        }

        for (auto TBToBBPair = TBToBB.begin(), End = TBToBB.end(); TBToBBPair != End;
             TBToBBPair = TBToBB.upper_bound(TBToBBPair->first)) {
            Function *F = TBToBBPair->first;
            F->eraseFromParent();
        }

        std::vector<Function *> entryPoints = fl.getEntrypointFunctions(m);
        auto *wrapper = cast<Function>(m.getOrInsertFunction("Func_wrapper", Type::getVoidTy(context)).getCallee());
        BasicBlock *entryBlock = BasicBlock::Create(context, "", wrapper);
        CallInst::Create(entryPoints[1], {}, "", entryBlock);
        ReturnInst::Create(context, entryBlock);

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addRecoverFunctionsPass(ModulePassManager &mpm) { mpm.addPass(RecoverFunctionsPass()); }
