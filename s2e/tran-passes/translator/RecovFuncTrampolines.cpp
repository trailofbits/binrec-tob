//
// Created by jn on 11/6/17.
//
#include "PassUtils.h"
#include "MetaUtils.h"
#include "DebugUtils.h"
#include "RecovFuncTrampolines.h"
#include "PcJumps.h"
#include "PcUtils.h"
#include "../../../plugins/util.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"
//#include "llvm/Analysis/Verifier.h"

using namespace llvm;

char RecovFuncTrampolinesPass::ID = 0;
static RegisterPass<RecovFuncTrampolinesPass> X("recov-func-trampolines",
      "insert trampolines for orig binary to call recovered functions", false, false);


struct OrigRecovFuncPair{
    u_int32_t origCallTargetAddress; //32 bit ptr
    u_int32_t origRetAddress;
    StoreInst *recovCallTargetAddress = NULL; //instr when store origCallTargetAddress to "PC"
    StoreInst *recovRetAddress = NULL; //instr when store origRetAddress to "PC", go back to trampoline after this

    OrigRecovFuncPair(unsigned octa, unsigned ora)
            :origCallTargetAddress(octa), origRetAddress(ora){
    }

    StoreInst * computeTargetAddress(Module &m, unsigned keyAddress){
        Function *wrapper = m.getFunction("wrapper");
        for(BasicBlock &B: *wrapper){
//            if(B.getName() =="BB_"+to_string(origCallTargetAddress)){
//
//            }
            for(Instruction &I: B){
                if(isInstStart(&I)){
                    if(auto *storeInst = dyn_cast<StoreInst>(&I)){
                        ifcast(ConstantInt,ci1,storeInst->getValueOperand()){
                            //DBG("store " << ci1);
                            if(ci1->getZExtValue() == keyAddress){
                                DBG("found store to pc " << ci1->getZExtValue());
                                return storeInst; //could we see it multiple times, i dont think so
                            }
                            else{
                              //  DBG("store mismatch, key, temp"<< origCallTargetAddress<< "    "<< ci1->getZExtValue());
                            }
                        }
                    }
                }
            }
    }
        return nullptr;
    }

//    bool computeRetAddress(Module &m){
//        Function *wrapper = m.getFunction("wrapper");
//        for(BasicBlock &B: *wrapper){
//            for(Instruction &I: B){
//                if(auto *loadInst = dyn_cast<LoadInst>(&I)){
//                    //DBG("load " << loadInst);
//                    if(loadInst->getName().count("pc") > 0 ) {//use other version when ported to 3.8
//                    //contains added after 3.2 if(loadInst->getName().contains("pc")) {//looks like %pc420, %pc322
//                        //DBG("inst name  " << loadInst->getName());
//                        ifcast(ConstantInt, ci1, loadInst) {
//
//                            DBG("cur  key  " << ci1->getZExtValue()<<"    " << origRetAddress);
//                            if (ci1->getZExtValue() == origRetAddress) {
//                                DBG("found load from pc" << ci1->getZExtValue());
//                                recovRetAddress = loadInst;
//                                return true; //could we see it multiple times, i dont think so
//                            }
//                        }
//                    }
//                }
//
//            }}
//        return false;
//    }

};

static bool readInFuncs(std::vector<OrigRecovFuncPair> *out,const std::string filename){
    std::ifstream inFile;
    s2eOpen(inFile,filename); //dont know if need to call close when using this
    if (!inFile) {
        //cout << "Unable to open file";
        DBG("Unable to open file");
        return false; // terminate with error
    }
    int line_num = 0;
    std::string line, line2;
    while (inFile && std::getline( inFile, line )) {
        //line_num++;
        if(std::getline( inFile, line2 )) {
            //DBG("looking up address pair");
            //DBG("ca "<<hexToInt(line)<<" ra "<< hexToInt(line));
            out->push_back(OrigRecovFuncPair(hexToInt(line),hexToInt(line2)));
        }
        else{
            //mismatched line numbers
            DBG("mismatched line numbers");
            inFile.close();
            return false;
        }
    }
    inFile.close();
    return true;
}

static bool writeOutFuncsToOverwrite(std::vector<OrigRecovFuncPair> *out,const std::string filename){
    std::ofstream outFile(filename);
    if (!outFile.is_open()) {
        //cout << "Unable to open file";
        return false; // terminate with error
    }
    foreach(item , *out) {
        outFile << item->origCallTargetAddress << "\n";
    }
    outFile.close();
    return true;
}

//static BasicBlock* getBlockByName(StringRef name, Function* containingF){
    //for (auto iter = containingF->getBasicBlockList().begin();
         //iter != containingF->getBasicBlockList().end();
         //iter++) {
        //BasicBlock* currBB = *iter;
////        DBG("name of bb "<<currBB->getName());
        //if (currBB->getName().equals(name)){
            //return currBB;
        //}
    //}
    //DBG("found no bb called " << name );
    //return nullptr;
//}

static BasicBlock* makeEnterTramp(Module &m, OrigRecovFuncPair *recovFunc){
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper);
    BasicBlock * enterTrampoline = BasicBlock::Create(m.getContext(), "enterTrampoline"+ std::to_string(recovFunc->origCallTargetAddress), wrapper);
    //assuming call semantics are handled identically in recovered and original code, so only execute version in recovered code
    IRBuilder<> b(enterTrampoline);
    ConstantInt *magicNum = ConstantInt::get(IntegerType::get(m.getContext(), 32), recovFunc->origCallTargetAddress);
    DBG("magicNum is " << magicNum->getZExtValue());
    Value *args[] = {
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
    );
    b.CreateStore(ConstantInt::getTrue(m.getContext()),m.getNamedGlobal("onUnfallback"));
    buildWrite(b,"unfallingback at ");
    buildWriteHex(b,magicNum);
    buildWriteChar(b, '\n');
    BasicBlock *callTarget = recovFunc->recovCallTargetAddress->getParent();
    b.CreateBr(callTarget);
    b.CreateBr(enterTrampoline);

    //make the fallback jumptable the predecessor of the trampoline BBs, purpose is to prevent it dead code elim
    //BasicBlock* f32 = getBlockByName("jumptable",wrapper);
    //DBG(f32->getName());
    //SwitchInst *jumpTable = dyn_cast<SwitchInst>(f32->getTerminator());
    //ConstantInt *caseTag = ConstantInt::get(
                        //Type::getInt32Ty(enterTrampoline->getContext()), - recovFunc->origCallTargetAddress );
    ////TODO add predecessors in a way with no security implication, this currently smells bad to me
    //jumpTable->addCase(caseTag,enterTrampoline);
    //
    //dyn_cast<Value>(enterTrampoline)->setValueSubclassData(1); //this makes hasAddressTaken() return true

    DBG("created enterTrampoline" << recovFunc->origCallTargetAddress);
    return enterTrampoline;
}

static BasicBlock* makeExitTramp(Module &m){
    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper);
    BasicBlock * exitTrampoline = BasicBlock::Create(m.getContext(), "exitTrampoline", wrapper);
    IRBuilder<> exitB(exitTrampoline);

    exitB.CreateStore(ConstantInt::getFalse(m.getContext()),m.getNamedGlobal("onUnfallback"));
    //Pop globals to real regs
    Value *args2[] = {
        m.getNamedGlobal("R_EAX"),
        m.getNamedGlobal("R_EBX"),
        m.getNamedGlobal("R_ECX"),
        m.getNamedGlobal("R_EDX"),
        m.getNamedGlobal("R_ESI"),
        m.getNamedGlobal("R_EDI"),
        m.getNamedGlobal("R_ESP"),
        m.getNamedGlobal("R_EBP")
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
        "ret",
        "*m,*m,*m,*m,*m,*m,*m,*m", true, args2
    );
    exitB.CreateUnreachable();
    return exitTrampoline;
}

//this pass expects to be run near the end of the lifting pipeline because it looks for
//inststart metadata and named values "pc"
bool RecovFuncTrampolinesPass::runOnModule(Module &m) {
    //Create global bool onUnfallback = false
    GlobalVariable *gOnUnfallback = cast<GlobalVariable>(m.getOrInsertGlobal("onUnfallback", IntegerType::get(m.getContext(), 1)));
    gOnUnfallback->setLinkage(GlobalValue::CommonLinkage);
    ConstantInt* const_int_val = ConstantInt::getFalse(m.getContext());
    gOnUnfallback->setInitializer(const_int_val);
    //set alignment?

    //parse input file of functions identified in original binary
    std::vector<OrigRecovFuncPair> origFunctions;
    if( !readInFuncs(&origFunctions, "binary.functions")){
        DBG("error reading in original function list");
        return false;
    }

    //find llvm instructions for entry and exit of each recovered function corresponding to identified original functions
    std::vector<OrigRecovFuncPair> recovFunctions;
    for(OrigRecovFuncPair origFunc : origFunctions ){
        origFunc.recovCallTargetAddress = origFunc.computeTargetAddress(m,origFunc.origCallTargetAddress);
        origFunc.recovRetAddress = origFunc.computeTargetAddress(m, origFunc.origRetAddress);
        if(origFunc.recovCallTargetAddress && origFunc.recovRetAddress){
            recovFunctions.push_back( origFunc);
        }
    }

    DBG("num recov funcs "<<recovFunctions.size());

        BasicBlock *exitTrampoline = makeExitTramp( m);
    for(OrigRecovFuncPair recovFunc : recovFunctions){
        BasicBlock *enterTrampoline = makeEnterTramp( m, &recovFunc);

        //patch recovered IR to use have entry from enterTrap and exit to exitTramp
        Instruction * termOfFuncExitBlock = recovFunc.recovRetAddress->getParent()->getTerminator();
        IRBuilder<> callB(termOfFuncExitBlock);
        Value *onUnfallback = callB.CreateLoad(m.getNamedGlobal("onUnfallback"));
        //when we port to 3.8, use this instead of the following 2 lines
        TerminatorInst * thenT = llvm::SplitBlockAndInsertIfThen(onUnfallback,termOfFuncExitBlock,1);
        //llvm 3.2 api
        //Value *oldAPICmp = callB.CreateICmpEQ(onUnfallback, ConstantInt::getTrue(m.getContext()));
        //TerminatorInst * thenT = llvm::SplitBlockAndInsertIfThen(dyn_cast<Instruction>(oldAPICmp),1, nullptr);

        BranchInst *goToExit = BranchInst::Create(exitTrampoline);
        llvm::ReplaceInstWithInst(thenT, goToExit);
        //IRBuilder<> toTrampoline(thenT);
        //toTrampoline.CreateBr(exitTrampoline);
        DBG("created exitTrampoline"<< recovFunc.origRetAddress);
        //do we need phis?
    }

    //verifyModule not implemented in 3.2?
    //if(verifyModule(m)){
        //DBG("Bad IR generated by RecovFuncTrampolines pass");
        //return false;
    //}

    if(!writeOutFuncsToOverwrite(&recovFunctions, "rfuncs")){
        DBG("error writing out funcs to overwrite")
        return false;
    }

    return true;
}
