#include "llvm/Analysis/CFG.h"
#include "llvm/IR/InlineAsm.h"

#include "PcJumps.h"
#include "PassUtils.h"
#include "MetaUtils.h"
#include "DebugUtils.h"
#include "PcUtils.h"

using namespace llvm;

char PcJumpsPass::ID = 0;
static RegisterPass<PcJumpsPass> X("pc-jumps",
        "S2E Replace stores to PC followed by returns with branches", false, false);

static void insertPrevPC(Function *wrapper, GlobalVariable *prevPC){
    for (BasicBlock &bb : *wrapper) {
        if (isRecoveredBlock(&bb)){
            //CallInst::Create(helper_debug_state)->insertAfter(getFirstInstStart(&bb));
            StoreInst *pcStore = getFirstInstStart(&bb);
            StoreInst *prevPcStore = new StoreInst(pcStore->getValueOperand(), prevPC,
							pcStore->isVolatile(), pcStore->getAlignment());
            prevPcStore->insertAfter(pcStore);
        }
    }
}

static BasicBlock *createErrorBlock(Module &m, Function *wrapper) {
    BasicBlock *errBlock = BasicBlock::Create(m.getContext(), "error", wrapper);
    GlobalVariable *PC = m.getNamedGlobal("PC");

    // jump to original .text section
    IRBuilder<> b(errBlock);

    switch (fallbackMode) {
    case fallback::NONE:
    case fallback::UNFALLBACK:
        b.CreateUnreachable();
        break;
    case fallback::ERROR1:
    {
        //create a prevPC global variable
        GlobalVariable *prevPC = cast<GlobalVariable>(m.getOrInsertGlobal("prevPC", PC->getType()->getElementType()));
        prevPC->setInitializer(PC->getInitializer());
        prevPC->setLinkage(PC->getLinkage());
        insertPrevPC(wrapper, prevPC);
        buildWrite(b, "prevPC: ");
        buildWriteHex(b, b.CreateLoad(prevPC));
    }
    case fallback::ERROR:
        buildWrite(b, " PC: ");
        buildWriteHex(b, b.CreateLoad(PC));
        buildWrite(b, "\nerror: the edge above doesn't exist, exiting\n");
        b.CreateStore(b.getInt32(-1), m.getNamedGlobal("R_EAX"));
        b.CreateRetVoid();
        break;
    case fallback::BASIC:
    case fallback::EXTENDED:
//    case fallback::UNFALLBACK:
        if (debugVerbosity >= 1) {
            buildWrite(b, "jumping to fallback for PC ");
            buildWriteHex(b, b.CreateLoad(PC));
            buildWriteChar(b, '\n');
        }

        Value *args[] = {
            m.getNamedGlobal("R_EAX"),
            m.getNamedGlobal("R_EBX"),
            m.getNamedGlobal("R_ECX"),
            m.getNamedGlobal("R_EDX"),
            m.getNamedGlobal("R_ESI"),
            m.getNamedGlobal("R_EDI"),
            m.getNamedGlobal("R_ESP"),
            m.getNamedGlobal("R_EBP"),
            b.CreateLoad(PC)
        };
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
            "*m,*m,*m,*m,*m,*m,*m,*m,r", true, args
        );
        b.CreateUnreachable();
        break;
    }

    return errBlock;
}

static bool isPossibleJumpTarget(BasicBlock *bb) {
    return isRecoveredBlock(bb) &&
        bb->getName().size() <= 11 &&
        bb != &bb->getParent()->getEntryBlock();
}

static SwitchInst *createJumpTable(BasicBlock *errBlock) {
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
    BasicBlock *jumpTableBlock =
            BasicBlock::Create(wrapper->getContext(), "jumptable", wrapper);
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
/*
static void createJumpTableEntry(SwitchInst *jumpTable, BasicBlock *bb) {
    ConstantInt *addr = ConstantInt::get(
            Type::getInt32Ty(bb->getContext()),
            getBlockAddress(bb));
    jumpTable->addCase(addr, bb);
}
*/
static bool findSuccs(BasicBlock *bb, std::vector<BasicBlock*> &succs) {
    while (!isRecoveredBlock(bb)) {
        pred_iterator it = pred_begin(bb);

        if (it == pred_end(bb))
            return false;

        bb = *it;
    }

    return getBlockSuccs(bb, succs);
}

/*
 * For each BB_xxxxxx in @wrapper, replace:
 *   ret void !{blockaddress(@wrapper, %BB_xxxxxx), ...}
 * with:
 *   %pc = load i32* @PC
 *   switch %pc, label %error [
 *       i32 xxxxxx, label %BB_xxxxxx
 *       ...
 *   ]
 */
bool PcJumpsPass::runOnModule(Module &m) {
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper);

    BasicBlock *errBlock = createErrorBlock(m, wrapper);

    GlobalVariable *PC = m.getNamedGlobal("PC");

    for (BasicBlock &bb : *wrapper) {
        ReturnInst *ret = dyn_cast<ReturnInst>(bb.getTerminator());
        std::vector<BasicBlock*> succs;

        if (ret && findSuccs(&bb, succs) && !succs.empty()) {
            LoadInst *loadPc = new LoadInst(PC, "pc", ret);
            SwitchInst *sw = SwitchInst::Create(loadPc, errBlock, succs.size(), ret);

            for (BasicBlock *succ : succs) {
                assert(isRecoveredBlock(succ));
                ConstantInt *addr = ConstantInt::get(
                        Type::getInt32Ty(bb.getContext()),
                        getBlockAddress(succ));
                sw->addCase(addr, succ);
            }

            sw->setMetadata("lastpc", ret->getMetadata("lastpc"));
            sw->setMetadata("inlined_lib_func", ret->getMetadata("inlined_lib_func"));

            ret->eraseFromParent();
        }
    }

    if (fallbackMode == fallback::EXTENDED) {
        SwitchInst *jumpTable = createJumpTable(errBlock);

        for (BasicBlock &bb : *wrapper) {
            if (isPossibleJumpTarget(&bb))
                createJumpTableEntry(jumpTable, &bb);
        }
    }

    return true;
}
