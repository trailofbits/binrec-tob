#define DEBUG_TYPE "loop-unroll"
#include "custom_loop_unroll.hpp"
#include "error.hpp"
#include "pass_utils.hpp"
#include <llvm/Analysis/LoopIterator.h>
#include <llvm/Analysis/ScalarEvolution.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>

using namespace llvm;

char CustomLoopUnroll::ID = 0;
static RegisterPass<CustomLoopUnroll>
    X("custom-loop-unroll", "S2E Unroll a loop a given number of times", false, false);

cl::opt<unsigned> UnrollCount(
    "custom-unroll-count",
    cl::desc("Number of times to unroll the loop"),
    cl::value_desc("count"));

cl::opt<std::string> EntryLabel(
    "custom-unroll-entry",
    cl::desc("Label of entry block for loop to unroll"),
    cl::value_desc("label"));

auto CustomLoopUnroll::doInitialization(Loop *L, LPPassManager &LPM) -> bool
{
    bool HaveErr = false;
    std::string error;

    if (UnrollCount.getNumOccurrences() != 1) {
        error = "error: please specify one -custom-unroll-count";
        HaveErr = true;
    }

    if (EntryLabel.getNumOccurrences() != 1) {
        error = "error: please specify one -custom-unroll-entry";
        HaveErr = true;
    }

    if (HaveErr) {
        throw binrec::lifting_error{"custom_loop_unroll", error};
    }

    return false;
}

// Convert the instruction operands from referencing the current values into
// those specified by VMap
// void RemapInstruction(Instruction *I, ValueToValueMapTy &VMap) {
//  for (unsigned op = 0, E = I->getNumOperands(); op != E; ++op) {
//    Value *Op = I->getOperand(op);
//    ValueToValueMapTy::iterator It = VMap.find(Op);
//    if (It != VMap.end())
//      I->setOperand(op, It->second);
//  }
//
//  if (PHINode *PN = dyn_cast<PHINode>(I)) {
//    for (unsigned i = 0, e = PN->getNumIncomingValues(); i != e; ++i) {
//      ValueToValueMapTy::iterator It = VMap.find(PN->getIncomingBlock(i));
//      if (It != VMap.end())
//        PN->setIncomingBlock(i, cast<BasicBlock>(It->second));
//    }
//  }
//}

auto ReplaceSuccessor(BasicBlock *BB, BasicBlock *Succ, BasicBlock *Repl) -> bool
{
    bool changed = false;
    if (auto *Br = dyn_cast<BranchInst>(BB->getTerminator())) {
        for (unsigned i = 0, e = Br->getNumSuccessors(); i != e; ++i) {
            if (Br->getSuccessor(i) == Succ) {
                Br->setSuccessor(i, Repl);
                changed = true;
            }
        }
    }
    return changed;
}

#define FAILIF(cond, msg)                                                                          \
    if (cond) {                                                                                    \
        LLVM_DEBUG(dbgs() << msg);                                                                 \
        return false;                                                                              \
    }

auto UnrollNTimes(Loop *L, LPPassManager &LPM, unsigned UnrollCount, LoopInfoWrapperPass &LIWP)
    -> bool
{
    LLVM_DEBUG(
        dbgs() << "Unroll " << UnrollCount << " iterations of loop with entry block " << EntryLabel
               << "\n");

    BasicBlock *PreHeader = L->getLoopPreheader();
    FAILIF(!PreHeader, "  Can't unroll; loop preheader-insertion failed.\n");

    BasicBlock *LatchBlock = L->getLoopLatch();
    FAILIF(!LatchBlock, "  Can't unroll; loop exit-block-insertion failed.\n");

    // Loops with indirectbr cannot be cloned.
    FAILIF(!L->isSafeToClone(), "  Can't unroll; Loop body cannot be cloned.\n");

    BasicBlock *Header = L->getHeader();
    FAILIF(Header->hasAddressTaken(), "  Won't unroll loop: address of header block is taken.\n");

    auto *BI = dyn_cast<BranchInst>(LatchBlock->getTerminator());
    FAILIF(
        !BI || BI->isConditional(),
        "  Cander't unroll; loop not terminated by an unconditional branch.\n");

    // For the first iteration of the loop, we should use the precloned values for
    // PHI nodes.  Insert associations now.
    ValueToValueMapTy LastValueMap;
    std::vector<PHINode *> OrigPHINodes;
    for (BasicBlock::iterator I = Header->begin(); isa<PHINode>(I); ++I)
        OrigPHINodes.push_back(cast<PHINode>(I));

    // The current on-the-fly SSA update requires blocks to be processed in
    // reverse postorder so that LastValueMap contains the correct value at each
    // exit.
    LoopBlocksDFS DFS(L);
    LoopInfo &LI = LIWP.getLoopInfo();
    DFS.perform(&LI);

    // Stash the DFS iterators before adding blocks to the loop.
    auto BlockBegin = DFS.beginRPO();
    auto BlockEnd = DFS.endRPO();

    // Insert a loop body before the original loop for each unroll interation
    for (unsigned It = 1; It != UnrollCount; ++It) {
        std::vector<BasicBlock *> NewBlocks;

        for (auto BB = BlockBegin; BB != BlockEnd; ++BB) {
            ValueToValueMapTy VMap;
            BasicBlock *New = CloneBasicBlock(*BB, VMap, "." + Twine(It));
            Header->getParent()->getBasicBlockList().push_back(New);

            // Loop over all of the PHI nodes in the block, changing them to use the
            // incoming values from the previous block.
            if (*BB == Header) {
                if (It == 1) {
                    for (PHINode *OrigPHI : OrigPHINodes) {
                        auto *NewPHI = cast<PHINode>(VMap[OrigPHI]);
                        VMap[OrigPHI] = NewPHI->getIncomingValueForBlock(PreHeader);
                        New->getInstList().erase(NewPHI);
                    }

                    ::ReplaceSuccessor(PreHeader, *BB, New);
                } else {
                    for (PHINode *OrigPHI : OrigPHINodes) {
                        auto *NewPHI = cast<PHINode>(VMap[OrigPHI]);
                        Value *InVal = NewPHI->getIncomingValueForBlock(LatchBlock);

                        if (auto *InValI = dyn_cast<Instruction>(InVal))
                            if (L->contains(InValI))
                                InVal = LastValueMap[InValI];

                        VMap[OrigPHI] = InVal;
                        New->getInstList().erase(NewPHI);
                    }

                    auto *PrevHeader = cast<BasicBlock>(LastValueMap[Header]);
                    auto *PrevLatch = cast<BasicBlock>(LastValueMap[LatchBlock]);
                    ::ReplaceSuccessor(PrevLatch, PrevHeader, New);
                }
            }

            // Update our running map of newest clones
            LastValueMap[*BB] = New;
            for (auto VI : VMap)
                LastValueMap[VI->first] = VI->second;

            // Add phi entries for newly created values to all exit blocks.
            for (succ_iterator SI = succ_begin(*BB), SE = succ_end(*BB); SI != SE; ++SI) {
                if (L->contains(*SI))
                    continue;
                for (BasicBlock::iterator BBI = (*SI)->begin(); auto *phi = dyn_cast<PHINode>(BBI);
                     ++BBI) {
                    Value *Incoming = phi->getIncomingValueForBlock(*BB);
                    ValueToValueMapTy::iterator It = LastValueMap.find(Incoming);
                    if (It != LastValueMap.end())
                        Incoming = It->second;
                    phi->addIncoming(Incoming, New);
                }
            }

            NewBlocks.push_back(New);
        }

        // Remap all instructions in the most recent iteration
        for (BasicBlock *BB : NewBlocks)
            for (Instruction &I : *BB)
                ::RemapInstruction(&I, LastValueMap);
    }

    //
    for (PHINode *OrigPHI : OrigPHINodes) {
        auto *LastLatch = cast<BasicBlock>(LastValueMap[LatchBlock]);
        OrigPHI->addIncoming(LastValueMap[OrigPHI], LastLatch);
        OrigPHI->removeIncomingValue(PreHeader);
    }

    auto *PrevHeader = cast<BasicBlock>(LastValueMap[Header]);
    auto *PrevLatch = cast<BasicBlock>(LastValueMap[LatchBlock]);
    ::ReplaceSuccessor(PrevLatch, PrevHeader, Header);

    // Stop verifier from complaining
    LI.erase(L);

    return true;
}

auto CustomLoopUnroll::runOnLoop(Loop *L, LPPassManager &LPM) -> bool
{
    if (!L->getHeader()->hasName() || L->getHeader()->getName() != EntryLabel)
        return false;

    return UnrollNTimes(L, LPM, UnrollCount, getAnalysis<LoopInfoWrapperPass>());
}

#undef FAILIF

// This transformation requires natural loop information & requires that loop
// preheaders be inserted into the CFG...
void CustomLoopUnroll::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequiredID(LoopSimplifyID);
    AU.addRequiredID(LCSSAID);
    AU.addRequired<ScalarEvolutionWrapperPass>();
}
