#include "Analysis/TraceInfoAnalysis.h"
#include "DebugUtils.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "RegisterPasses.h"
#include "Utils/FunctionInfo.h"
#include <llvm/Analysis/CFG.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/InlineAsm.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
void insertPrevPC(Function *wrapper, GlobalVariable *prevPC) {
    for (BasicBlock &bb : *wrapper) {
        if (isRecoveredBlock(&bb)) {
            // CallInst::Create(helper_debug_state)->insertAfter(getFirstInstStart(&bb));
            StoreInst *pcStore = getFirstInstStart(&bb);
            new StoreInst(pcStore->getValueOperand(), prevPC, pcStore->isVolatile(), pcStore->getNextNode());
        }
    }
}

auto createErrorBlock(Module &m, Function *wrapper) -> BasicBlock * {
    BasicBlock *errBlock = BasicBlock::Create(m.getContext(), "error", wrapper);
    GlobalVariable *pc = m.getNamedGlobal("PC");

    // jump to original .text section
    IRBuilder<> b(errBlock);

    switch (fallbackMode) {
    case fallback::NONE:
    case fallback::UNFALLBACK:
        b.CreateUnreachable();
        break;
    case fallback::ERROR1: {
        // create a prevPC global variable
        auto *prevPC = cast<GlobalVariable>(m.getOrInsertGlobal("prevPC", pc->getType()->getElementType()));
        prevPC->setInitializer(pc->getInitializer());
        prevPC->setLinkage(pc->getLinkage());
        insertPrevPC(wrapper, prevPC);
        buildWrite(b, "prevPC: ");
        buildWriteHex(b, b.CreateLoad(prevPC));
    }
    case fallback::ERROR:
        buildWrite(b, " PC: ");
        buildWriteHex(b, b.CreateLoad(pc));
        buildWrite(b, "\nerror: the edge above doesn't exist, exiting\n");
        b.CreateStore(b.getInt32(-1), m.getNamedGlobal("R_EAX"));
        b.CreateRetVoid();
        break;
    case fallback::BASIC:
    case fallback::EXTENDED:
        //    case fallback::UNFALLBACK:
        if (debugVerbosity >= 1) {
            buildWrite(b, "jumping to fallback for PC ");
            buildWriteHex(b, b.CreateLoad(pc));
            buildWriteChar(b, '\n');
        }

        array<Value *, 9> args = {m.getNamedGlobal("R_EAX"), m.getNamedGlobal("R_EBX"), m.getNamedGlobal("R_ECX"),
                                  m.getNamedGlobal("R_EDX"), m.getNamedGlobal("R_ESI"), m.getNamedGlobal("R_EDI"),
                                  m.getNamedGlobal("R_ESP"), m.getNamedGlobal("R_EBP"), b.CreateLoad(pc)};
        doInlineAsm(b,
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
                    "*m,*m,*m,*m,*m,*m,*m,*m,r", true, args);
        b.CreateUnreachable();
        break;
    }

    return errBlock;
}

auto isPossibleJumpTarget(BasicBlock *bb) -> bool {
    return isRecoveredBlock(bb) && bb->getName().size() <= 11 && bb != &bb->getParent()->getEntryBlock();
}

auto createJumpTable(BasicBlock *errBlock) -> SwitchInst * {
    Function *wrapper = errBlock->getParent();

    // find error block and count recovered blocks
    unsigned numCases = 0;
    for (BasicBlock &bb : *wrapper) {
        if (isPossibleJumpTarget(&bb))
            numCases++;
    }
    assert(numCases > 0);

    DBG("creating BB jump table of size " << numCases);

    // create new jump table block which replaces the error block
    BasicBlock *jumpTableBlock = BasicBlock::Create(wrapper->getContext(), "jumptable", wrapper);
    errBlock->replaceAllUsesWith(jumpTableBlock);

    IRBuilder<> b(jumpTableBlock);
    Value *loadPc = b.CreateLoad(wrapper->getParent()->getNamedGlobal("PC"));

    // add runtime message informing user about jumptable fallback
    if (debugVerbosity >= 1) {
        buildWrite(b, "PC ");
        buildWriteHex(b, loadPc);
        buildWrite(b, " not in original code path, trying jumptable\n");
    }

    // add a switch statement on the value of @PC that defaults to the old
    // error block
    return b.CreateSwitch(loadPc, errBlock, numCases);
}

// void createJumpTableEntry(SwitchInst *jumpTable, BasicBlock *bb) {
//     ConstantInt *addr = ConstantInt::get(
//             Type::getInt32Ty(bb->getContext()),
//             getBlockAddress(bb));
//     jumpTable->addCase(addr, bb);
// }

auto findMasterBlock(BasicBlock *bb) -> BasicBlock * {
    while (!isRecoveredBlock(bb)) {
        pred_iterator it = pred_begin(bb);

        if (it == pred_end(bb))
            return nullptr;

        bb = *it;
    }

    return bb;
}

/// S2E Replace stores to PC followed by returns with branches
class PcJumpsPass : public PassInfoMixin<PcJumpsPass> {
public:
    /// For each BB_xxxxxx in @wrapper, replace:
    ///   ret void !{blockaddress(@wrapper, %BB_xxxxxx), ...}
    /// with:
    ///   %pc = load i32* @PC
    ///   switch %pc, label %error [
    ///       i32 xxxxxx, label %BB_xxxxxx
    ///       ...
    ///   ]
    // NOLINTNEXTLINE
    auto run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
        TraceInfo &ti = mam.getResult<TraceInfoAnalysis>(m);
        FunctionInfo fi{ti};

        for (Function &f : m) {
            if (!f.getName().startswith("Func_"))
                continue;

            BasicBlock *errBb = createErrorBlock(m, &f);

            GlobalVariable *pc = m.getNamedGlobal("PC");

            for (BasicBlock &bb : f) {
                auto *ret = dyn_cast<ReturnInst>(bb.getTerminator());
                vector<BasicBlock *> succs;
                BasicBlock *masterBlock = findMasterBlock(&bb);
                if (!masterBlock)
                    continue;

                getBlockSuccs(masterBlock, succs);

                if (!ret || succs.empty())
                    continue;

                bool isRetBlock =
                    ti.functionLog.entryToReturn.find(make_pair(getPc(&f.getEntryBlock()), getPc(masterBlock))) !=
                    ti.functionLog.entryToReturn.end();
                // FPar: This loop is still in there, because I am not sure whether trace info will find all rets.
                for (BasicBlock *succ : succs) {
                    if (succ->getParent() != &f) {
                        assert(isRetBlock);
                        isRetBlock = true;
                        break;
                    }
                }

                if (isRetBlock)
                    continue;

                auto *loadPc = new LoadInst(pc->getValueType(), pc, "pc", ret);
                SwitchInst *sw = SwitchInst::Create(loadPc, errBb, succs.size(), ret);

                for (BasicBlock *succ1 : succs) {
                    assert(isRecoveredBlock(succ1));
                    ConstantInt *addr = ConstantInt::get(Type::getInt32Ty(bb.getContext()), getBlockAddress(succ1));
                    sw->addCase(addr, succ1);
                }

                sw->setMetadata("lastpc", ret->getMetadata("lastpc"));
                sw->setMetadata("inlined_lib_func", ret->getMetadata("inlined_lib_func"));

                ret->eraseFromParent();
            }

            for (BasicBlock &bb : f) {
                // Remove metadata here, because it simplifies later passes.
                setBlockSuccs(&bb, {});
            }

            if (fallbackMode == fallback::EXTENDED) {
                SwitchInst *jumpTable = createJumpTable(errBb);

                for (BasicBlock &bb : f) {
                    if (isPossibleJumpTarget(&bb))
                        createJumpTableEntry(jumpTable, &bb);
                }
            }
        }
        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addPcJumpsPass(ModulePassManager &mpm) { mpm.addPass(PcJumpsPass()); }
