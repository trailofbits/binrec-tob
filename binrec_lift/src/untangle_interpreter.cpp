#include "untangle_interpreter.hpp"
#include "custom_loop_unroll.hpp"
#include "pass_utils.hpp"
#include <fstream>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Utils.h>
#include <set>

using namespace llvm;

char UntangleInterpreter::ID = 0;
static RegisterPass<UntangleInterpreter>
    X("untangle-interpreter",
      "S2E Untangle interpreter loop for each distinct virtual program counter value",
      false,
      false);

cl::opt<std::string> InterpreterEntry(
    "interpreter-entry",
    cl::desc("Label of entry block of interpreter loop"),
    cl::value_desc("label"));

cl::opt<std::string> VpcGlobal(
    "vpc-global",
    cl::desc("Name of virtual program counter global"),
    cl::value_desc("global"));

cl::list<std::string> VpcTraceFiles(
    "vpc-trace",
    cl::desc("Trace files of instrumented interpreter runs"),
    cl::value_desc("file"));

static auto isInterpreterLoop(Loop *L) -> bool
{
    if (InterpreterEntry.getNumOccurrences() != 1) {
        errs() << "error: please specify one -interpreter-entry\n";
        exit(1);
    }

    return L->getHeader()->hasName() && L->getHeader()->getName() == InterpreterEntry;
}

static auto findVpcGlobal(Module &m) -> GlobalVariable *
{
    if (VpcGlobal.getNumOccurrences() != 1) {
        errs() << "error: please specify one -vpc-global\n";
        return nullptr;
    }

    GlobalVariable *vpc = m.getNamedGlobal(VpcGlobal);

    if (!vpc)
        errs() << "Error: no global called @" << VpcGlobal << "\n";

    return vpc;
}

static auto readTraceFiles(
    unsigned &entry,
    std::set<unsigned> &exits,
    std::set<std::pair<unsigned, unsigned>> &edges) -> bool
{
    if (VpcTraceFiles.getNumOccurrences() < 1) {
        errs() << "Error: please specify at least one -vpc-trace\n";
        return false;
    }

    bool entrySet = false;

    for (std::string filename : VpcTraceFiles) {
        std::ifstream f;
        s2eOpen(f, filename, true);
        unsigned prev = 0, cur = 0;

        // Assert that each trace has the same start VPC
        if (f >> std::dec >> cur) {
            if (entrySet) {
                if (cur != entry) {
                    errs() << "Error: different start VPCs: " << entry << " and " << cur << "\n";
                    return false;
                }
                prev = entry;
            } else {
                entry = prev = cur;
                entrySet = true;
            }
        } else {
            errs() << "Error: trace file \"" << filename << "\" is empty\n";
            return false;
        }

        while (f >> std::dec >> cur) {
            edges.insert(std::make_pair(prev, cur));
            prev = cur;
        }

        exits.insert(cur);
    }

    return true;
}

static void cloneLoopBody(Loop *L, unsigned vpcValue, ValueToValueMapTy &VMap)
{
    // Copy all basic blocks in the loop and remap instructions
    auto &bbList = L->getHeader()->getParent()->getBasicBlockList();
    std::vector<BasicBlock *> newBlocks;

    for (BasicBlock *bb : L->getBlocks()) {
        BasicBlock *newBlock = CloneBasicBlock(bb, VMap, "." + Twine(vpcValue));
        bbList.push_back(newBlock);
        newBlocks.push_back(newBlock);
        VMap[bb] = newBlock;
    }

    for (BasicBlock *bb : newBlocks)
        for (Instruction &inst : *bb)
            RemapInstruction(&inst, VMap, RF_NoModuleLevelChanges | RF_IgnoreMissingLocals);
}

auto UntangleInterpreter::getOrCreatePair(const unsigned vpcValue) -> const HeaderLatchPair &
{
    auto it = vpcMap.find(vpcValue);

    if (it == vpcMap.end()) {
        ValueToValueMapTy VMap;
        cloneLoopBody(interpreter, vpcValue, VMap);
        auto *newHeader = cast<BasicBlock>(VMap[interpreter->getHeader()]);
        auto *newLatch = cast<BasicBlock>(VMap[interpreter->getLoopLatch()]);

        // Replace backedge with switch statement
        auto *br = dyn_cast<BranchInst>(newLatch->getTerminator());
        assert(br);
        assert(br->getNumSuccessors() == 1);
        assert(br->getSuccessor(0) == newHeader);
        br->eraseFromParent();

        // Set VPC constant at start of header block to help dead code
        // elimination
        IRBuilder<> b(&*newHeader->begin());
        b.CreateStore(b.getInt32(vpcValue), vpc);

        if (!errBlock) {
            errBlock = BasicBlock::Create(newLatch->getContext(), "error", newLatch->getParent());
            b.SetInsertPoint(errBlock);
            b.CreateUnreachable();
        }

        b.SetInsertPoint(newLatch);
        b.CreateSwitch(b.CreateLoad(vpc), errBlock, 2);

        HeaderLatchPair pair = std::make_pair(newHeader, newLatch);
        return vpcMap.insert(std::make_pair(vpcValue, pair)).first->second;
    } else {
        return it->second;
    }
}

auto UntangleInterpreter::runOnLoop(Loop *L, LPPassManager &LPM) -> bool
{
    if (!isInterpreterLoop(L))
        return false;

    interpreter = L;
    Module *m = L->getHeader()->getParent()->getParent();
    vpc = findVpcGlobal(*m);
    unsigned entry = 0;
    std::set<unsigned> exits;
    std::set<std::pair<unsigned, unsigned>> edges;
    const bool filesRead = readTraceFiles(entry, exits, edges);

    if (!vpc || !filesRead)
        exit(1);

    // The header cannot contain PHI nodes because this will create problems
    // when pointing successors to other nodes
    if (isa<PHINode>(L->getHeader()->begin())) {
        errs() << "Error: interpreter header should not contain PHI nodes\n";
        exit(1);
    }

    auto *vpcTy = cast<IntegerType>(vpc->getType()->getElementType());

    // Point preheader to first virtual instruction
    const HeaderLatchPair &entryPair = getOrCreatePair(entry);

    if (!ReplaceSuccessor(L->getLoopPreheader(), L->getHeader(), entryPair.first)) {
        errs() << "Error: could not patch successor of preheader\n";
        exit(1);
    }

    // Add edges between virtual instructions based on trace files, duplicating
    // the interpreter loop body once for each virtual instruction
    for (auto &iedge : edges) {
        const unsigned src = iedge.first, dst = iedge.second;

        const HeaderLatchPair &srcPair = getOrCreatePair(src);
        const HeaderLatchPair &dstPair = getOrCreatePair(dst);

        auto *sw = cast<SwitchInst>(srcPair.second->getTerminator());
        sw->addCase(ConstantInt::get(vpcTy, dst), dstPair.first);
    }

    // Connect virtualexit instructions to exit original interpreter loop. This
    // loop should be optimized away.
    for (const unsigned exitVpc : exits) {
        const HeaderLatchPair &srcPair = getOrCreatePair(exitVpc);
        auto *sw = cast<SwitchInst>(srcPair.second->getTerminator());
        sw->setDefaultDest(L->getHeader());
    }

    return true;
}

void UntangleInterpreter::getAnalysisUsage(AnalysisUsage &AU) const
{
    AU.addRequired<LoopInfoWrapperPass>();
    AU.addRequiredID(LoopSimplifyID);
    AU.addRequiredID(LCSSAID);
}
