#include "Analysis/TraceInfoAnalysis.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include "RegisterPasses.h"
#include "Utils/FunctionInfo.h"
#include <fstream>
#include <llvm/IR/CFG.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>
#include <map>
#include <set>
#include <unordered_set>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
auto getBlockByName(StringRef name, Function *containingF) -> BasicBlock * {
    for (auto &currBB : *containingF) {
        if (currBB.getName().equals(name)) {
            return &currBB;
        }
    }
    DBG("found no bb called " << name);
    return nullptr;
}

auto makeEnterTramp(Module &m, BasicBlock *entryBB) -> BasicBlock * {
    Function *wrapper = m.getFunction("Func_wrapper");
    assert(wrapper);
    uint32_t entryPc = getPc(entryBB);
    BasicBlock *enterTrampoline = BasicBlock::Create(m.getContext(), "enterTrampoline." + entryBB->getName(), wrapper);
    // assuming call semantics are handled identically in recovered and original
    // code, so only execute version in recovered code
    IRBuilder<> b(enterTrampoline);
    ConstantInt *magicNum = ConstantInt::get(IntegerType::get(m.getContext(), 32), entryPc);
    DBG("magicNum is " << magicNum->getZExtValue());
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
        magicNum,
    };
    doInlineAsm(b,
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
                "*m,i", true, args);

    b.CreateStore(ConstantInt::getTrue(m.getContext()), m.getNamedGlobal("onUnfallback"));
    // buildWrite(b,"unfallingback at ");
    // buildWriteHex(b,magicNum);
    // buildWriteChar(b, '\n');

    // make the fallback jumptable the predecessor of the trampoline BBs
    BasicBlock *entryPred = nullptr;
    for (BasicBlock *pred : predecessors(entryBB)) {
        if (pred_begin(pred) != pred_end(pred)) { // pred has predecessor so it will not get deleted
            entryPred = pred;
            break;
        }
    }

    auto *predSwitch = dyn_cast<SwitchInst>(entryPred->getTerminator());
    assert(predSwitch && "Terminator is not switch");
    ConstantInt *caseTag = ConstantInt::get(Type::getInt32Ty(enterTrampoline->getContext()), -entryPc);
    predSwitch->addCase(caseTag, enterTrampoline);
    // create branch from enterTrampoline to entry
    b.CreateBr(entryBB);

    DBG(*entryPred);
    DBG(*enterTrampoline);
    // DBG("created enterTrampoline" << recovFunc->origCallTargetAddress);
    return enterTrampoline;
}

auto makeExitTramp(Module &m, BasicBlock *returnBB) -> BasicBlock * {
    Function *wrapper = m.getFunction("Func_wrapper");
    assert(wrapper);
    BasicBlock *exitTrampoline = BasicBlock::Create(m.getContext(), "exitTrampoline", wrapper);
    IRBuilder<> exitB(exitTrampoline);

    exitB.CreateStore(ConstantInt::getFalse(m.getContext()), m.getNamedGlobal("onUnfallback"));
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
    doInlineAsm(exitB,
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
                "*m,*m,*m,*m", true, args2);
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
    exitB.CreateUnreachable();

    // patch recovered IR to use have entry from enterTrap and exit to exitTramp
    Instruction *returnBBTerm = returnBB->getTerminator();
    IRBuilder<> callB(returnBBTerm);
    Value *onUnfallback = callB.CreateLoad(m.getNamedGlobal("onUnfallback"));
    // Value *pc = callB.CreateLoad(m.getNamedGlobal("PC"));
    // when we port to 3.8, use this instead of the following 2 lines
    Instruction *thenT = llvm::SplitBlockAndInsertIfThen(onUnfallback, returnBBTerm, true);
    // Value *oldAPICmp = callB.CreateICmpEQ(onUnfallback,
    // ConstantInt::getTrue(m.getContext())); TerminatorInst * thenT =
    // llvm::SplitBlockAndInsertIfThen(dyn_cast<Instruction>(oldAPICmp),1,
    // nullptr);

    BranchInst *goToExit = BranchInst::Create(exitTrampoline);
    llvm::ReplaceInstWithInst(thenT, goToExit);
    return exitTrampoline;
}

/// insert trampolines for orig binary to call recovered functions
///
/// XXX: This pass currently assumes that all the callback functions have
/// returned from the same location.
/// The reason for this assumption is that, the metadata we collect from the
/// frontend is not enough to find all the return BBs inside a callback function.
/// We can actually find other return BBs with an assumption of callback function
/// never jumps to its entry BB. This assumption wouldn't hold for some of the
/// callback functions such as tail recursive functions. If the above assumption
/// holds, this is how we can find other return BBs in the callback functions:
/// Follow the succs starting from entry BB of callback function.
/// At every BB, check if any of the succs is entry BB. If it is, then the
/// current BB is return BB. Continue until we reach return BB that is in
/// entryToReturn.(key is address of lib function in plt. For example: qsort
/// takes compare function and return BB of compare function can be found using
/// address of qsort in plt)
class InsertTrampForRecFuncsPass : public PassInfoMixin<InsertTrampForRecFuncsPass> {
    unordered_set<uint32_t> callers;
    GlobalVariable *prevR_ESP{};

public:
    /// this pass expects to be run near the end of the lifting pipeline because it
    /// looks for inststart metadata and named values "pc"
    auto run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
        // TODO: find out why local calls in callback IR pass parameter by writing
        // to where R_ESP is pointing instead of pushing it to the stack.
        // TODO: phyton script jumps to beginning of enterTramp. Instead jump to
        // movl %esp R_ESP instruction.

        TraceInfo &ti = mam.getResult<TraceInfoAnalysis>(m);
        FunctionInfo fi{ti};

        // Create global bool onUnfallback = false
        auto *gOnUnfallback =
            cast<GlobalVariable>(m.getOrInsertGlobal("onUnfallback", IntegerType::get(m.getContext(), 1)));
        gOnUnfallback->setLinkage(GlobalValue::CommonLinkage);
        ConstantInt *const_int_val = ConstantInt::getFalse(m.getContext());
        gOnUnfallback->setInitializer(const_int_val);
        // set alignment?

        // create a prevR_ESP to store where it was pointing.
        GlobalVariable *R_ESP = m.getNamedGlobal("R_ESP");
        assert(R_ESP && "R_ESP global variable doesn't exist");
        prevR_ESP = cast<GlobalVariable>(m.getOrInsertGlobal("prevR_ESP", R_ESP->getType()->getElementType()));
        prevR_ESP->setInitializer(R_ESP->getInitializer());
        prevR_ESP->setLinkage(R_ESP->getLinkage());

        // parse input file of functions identified in original binary
        unordered_set<BasicBlock *> entries;
        unordered_set<BasicBlock *> returns;
        getCBEntryAndReturns(m, entries, returns, fi);

        for (BasicBlock *entryBB : entries) {
            makeEnterTramp(m, entryBB);
        }
        for (BasicBlock *returnBB : returns) {
            makeExitTramp(m, returnBB);
        }

        // Write func entry PC values to a file to be processed by patching script.
        ofstream outFile("rfuncs");
        if (!outFile.is_open()) {
            errs() << "Unable to open file";
            exit(1);
        }
        for (BasicBlock *entry : entries) {
            outFile << getPc(entry) << "\n";
        }
        outFile.close();
        return PreservedAnalyses::none();
    }

private:
    /// This function finds BBs(pcs) that are entry BBs and ret BBs. It doesn't
    /// match entry to ret. All we know is a BB is entry/return of some callback
    /// function.
    void getCBEntryAndReturns(Module &m, unordered_set<BasicBlock *> &returns, unordered_set<BasicBlock *> &entries,
                              const FunctionInfo &fi) {
        Function *wrapper = m.getFunction("Func_wrapper");
        assert(wrapper && "Func_wrapper not found");
        Function *trampoline = m.getFunction("helper_stub_trampoline");
        assert(trampoline && "trampoline not found");

        // iterate over lib functions
        for (auto *U : trampoline->users()) {
            DBG(*U);
            auto *I = cast<Instruction>(U);
            BasicBlock *BB = I->getParent();
            if (BB->getParent() != wrapper)
                continue;
            if (pred_begin(BB) == pred_end(BB))
                continue;

            uint32_t BBPc = getPc(BB);
            // Check if lib function takes callback func

            auto entryToBBsIt = fi.entryPcToBBPcs.find(BBPc);
            if (entryToBBsIt == fi.entryPcToBBPcs.end()) {
                DBG("No record for the lib function: " << intToHex(BBPc));
                continue;
            }

            if (entryToBBsIt->second.size() <= 3)
                continue;

            DBG("lib function pc that calls callback: " << intToHex(BBPc));

            auto it = fi.entryPcToReturnPcs.find(BBPc);
            assert(it != fi.entryPcToReturnPcs.end() && "No record for the lib function");

            for (uint32_t retPc : it->second) {
                // insert retBB into set
                DBG("callback ret: " << intToHex(retPc));
                BasicBlock *retBB = getBlockByName("BB_" + intToHex(retPc), wrapper);
                assert(retBB && "Couldn't find return BB");
                returns.insert(retBB);
            }

            for (auto *succ : successors(BB)) {
                uint32_t succPc = getPc(succ);
                // Means this succ(func entry) was not called by this lib func
                if (entryToBBsIt->second.find(succPc) == entryToBBsIt->second.end())
                    continue;
                bool discard = false;
                for (auto caller : callers) {
                    int diff = succPc - caller;
                    if (diff > 0 && diff < 16)
                        discard = true;
                }
                if (discard)
                    continue;
                // if(succPc == 134513843)
                // if(succPc == 134513809)
                //    continue;
                // insert succBB as entry of callback functions
                DBG("callback entry: " << intToHex(succPc));
                entries.insert(succ);
            }
        }

        for (auto *B : entries) {
            DBG(B->getName());
        }
        for (auto *B : returns) {
            DBG(B->getName());
        }
    }
};
} // namespace

void binrec::addInsertTrampForRecFuncsPass(ModulePassManager &mpm) { mpm.addPass(InsertTrampForRecFuncsPass()); }
