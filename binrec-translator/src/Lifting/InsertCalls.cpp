#include "Analysis/TraceInfoAnalysis.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include "Utils/FunctionInfo.h"
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <map>
#include <set>
#include <vector>

using namespace binrec;
using namespace llvm;

namespace {
/// This function deals with the slight mismatch of "recovered blocks" and the
/// fact that a recovered block can consist of multiple successors. When trying
/// to change the store to the last PC of a recovered block, we have to find the
/// last block the make up the recovered block. An example is a block that
/// contains a `setz` instruction. Such instructions are translated into a
/// branch followed by a phi. Look for "Func_XXXXXXX.exit" blocks.
auto findExitBlock(BasicBlock *bb) -> BasicBlock * {
    BasicBlock *exitBlock = bb;

    while (succ_begin(exitBlock) != succ_end(exitBlock)) {
        exitBlock = *succ_begin(exitBlock);
    }

    return exitBlock;
}

/// Insert calls between recovered functions.
class InsertCallsPass : public PassInfoMixin<InsertCallsPass> {
public:
    // NOLINTNEXTLINE
    auto run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
        TraceInfo &ti = mam.getResult<TraceInfoAnalysis>(m);
        FunctionInfo fi{ti};

        std::map<Function *, std::set<BasicBlock *>> allRetBlocks = fi.getRetBBsByMergedFunction(m);

        for (Function &f : m) {

            if (!f.getName().startswith("Func_") ||
                f.getEntryBlock().getTerminator()->getMetadata("extern_symbol") != nullptr) {
                continue;
            }

            std::set<BasicBlock *> &retBlocks = allRetBlocks[&f];

            for (BasicBlock &bb : f) {
                if (retBlocks.find(&bb) != retBlocks.end()) {
                    continue;
                }

                std::vector<BasicBlock *> successors;
                if (!getBlockSuccs(&bb, successors) || successors.empty()) {
                    continue;
                }

                // Q: Is it safe to assume that the entry block being a successor means there is a recursive call,
                // rather than a branch? A: Probably yes, because there should not be a branch from within the function
                // to its prologue?
                if (successors[0]->getParent() != &f || successors[0] == &f.getEntryBlock()) {
                    BasicBlock *exitBlock = findExitBlock(&bb);

                    BasicBlock *callFollowUpBlock = nullptr;

                    uint32_t callPc = getLastPc(&bb);
                    auto callFollowUpPc = fi.callerPcToFollowUpPc.find(callPc);
                    if (callFollowUpPc != fi.callerPcToFollowUpPc.end()) {
                        // When the call does not return (e.g. call to exit()) there is no follow up block.
                        std::string callFollowUpBlockName = std::string("BB_") + intToHex(callFollowUpPc->second);
                        for (BasicBlock &bb2 : f) {
                            if (bb2.getName() == callFollowUpBlockName) {
                                callFollowUpBlock = &bb2;
                            }
                        }
                        if (callFollowUpBlock == nullptr) {
                            errs() << "Can't find call follow up block " << callFollowUpBlockName << " of block "
                                   << bb.getName() << " in function " << f.getName() << ".\n";
                            assert(callFollowUpBlock);
                        }
                    }

                    if (successors.size() == 1) {
                        // Direct call
                        StoreInst *targetPcStore = findLastPCStore(*exitBlock);
                        if (targetPcStore == nullptr) {
                            errs() << "Can't find PC store on callsite in block " << bb.getName() << ".\n";
                            continue;
                        }
                        targetPcStore->eraseFromParent();

                        CallInst::Create(successors[0]->getParent(), {}, "", exitBlock->getTerminator());

                        StoreInst *callInstPcStore = getLastInstStart(exitBlock);
                        bb.getTerminator()->setMetadata(
                            "lastpc",
                            MDNode::get(bb.getContext(), ValueAsMetadata::get(callInstPcStore->getValueOperand())));

                        if (callFollowUpBlock == nullptr) {
                            exitBlock->getTerminator()->eraseFromParent();
                            new UnreachableInst(exitBlock->getContext(), exitBlock);
                            setBlockSuccs(&bb, {});
                        } else {
                            new StoreInst(getFirstInstStart(callFollowUpBlock)->getValueOperand(),
                                          m.getNamedGlobal("PC"), exitBlock->getTerminator());
                            std::vector<BasicBlock *> metaSuccs{callFollowUpBlock};
                            setBlockSuccs(&bb, metaSuccs);
                        }
                    } else {
                        unsigned lastPc = getLastPc(&bb);

                        // Create join block after calls
                        auto *terminator = cast<ReturnInst>(exitBlock->getTerminator());
                        BasicBlock *joinBlock = BasicBlock::Create(m.getContext(), bb.getName() + "_join", &f);
                        joinBlock->moveAfter(exitBlock);

                        if (callFollowUpBlock == nullptr) {
                            assert(false &&
                                   "This is not implemented yet. Also, is metadata set correctly with indirect calls?");
                        } else {
                            ReturnInst::Create(bb.getContext(), terminator->getReturnValue(), joinBlock);
                            new StoreInst(getFirstInstStart(callFollowUpBlock)->getValueOperand(),
                                          m.getNamedGlobal("PC"), joinBlock->getTerminator());
                            std::vector<BasicBlock *> metaSuccs{callFollowUpBlock};
                            setBlockSuccs(joinBlock, metaSuccs);
                            joinBlock->getTerminator()->setMetadata(
                                "lastpc",
                                MDNode::get(bb.getContext(), ValueAsMetadata::get(ConstantInt::get(
                                                                 IntegerType::getInt32Ty(bb.getContext()), lastPc))));
                        }

                        // Call of function pointer.
                        auto *pcGlobal = m.getNamedGlobal("PC");
                        auto *targetPcLoad =
                            new LoadInst(pcGlobal->getValueType(), pcGlobal, "", exitBlock->getTerminator());
                        BasicBlock *errBlock = BasicBlock::Create(m.getContext(), bb.getName() + "_error", &f);
                        errBlock->moveAfter(joinBlock);
                        new UnreachableInst(bb.getContext(), errBlock);
                        SwitchInst *callSwitch =
                            SwitchInst::Create(targetPcLoad, errBlock, successors.size(), exitBlock);

                        for (BasicBlock *successor : successors) {
                            assert((successor->getParent() != &f || successor == &f.getEntryBlock()) &&
                                   "Successor is basic block in own function instead of "
                                   "a function for function pointer call.");
                            Function *callee = successor->getParent();
                            BasicBlock *callBlock = BasicBlock::Create(
                                m.getContext(), std::string("call_") + callee->getName() + "_from_" + bb.getName(), &f);
                            callBlock->moveAfter(exitBlock);
                            BranchInst::Create(joinBlock, callBlock);

                            ConstantInt *successorPc =
                                ConstantInt::get(Type::getInt32Ty(bb.getContext()), getBlockAddress(successor));
                            callSwitch->addCase(successorPc, callBlock);

                            CallInst::Create(callee, {}, "", callBlock->getTerminator());
                        }

                        terminator->eraseFromParent();
                    }
                }
            }
        }
        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addInsertCallsPass(ModulePassManager &mpm) { mpm.addPass(InsertCallsPass()); }
