#include <iostream>
#include <algorithm>

#include "FixCFG.h"
#include "PassUtils.h"
#include "PcUtils.h"
#include "MetaUtils.h"

using namespace llvm;

char FixCFG::ID = 0;
static RegisterPass<FixCFG> X("fix-cfg",
        "Fix cfg by finding escaping pointers",
        false, false);

// This pass works only if library functions that take local pointer are reached by call instruction.
// If library function is reached by jmp instruction, this pass will fail.


void FixCFG::loadFunctionMetadata(){
    std::ifstream f;
    uint32_t func_start, func_end, caller, followUp;
    s2eOpen(f, "entryToCaller");

    while (f.read((char*)&caller, 4).read((char*)&func_start, 4)) {
        errs() << "func_start: " << intToHex(func_start) << " caller: " << intToHex(caller) << "\n";
        errs() << "func_start: " << func_start << " caller: " << caller << "\n";

        entryPcToCallerPcs[func_start].insert(caller);
    }

    std::ifstream ff;
    s2eOpen(ff, "entryToReturn");

    while (ff.read((char*)&func_end, 4).read((char*)&func_start, 4)) {
        errs() << "func_start: " << intToHex(func_start) << " func_end: " << intToHex(func_end) << "\n";
        errs() << "func_start: " << func_start << " func_end: " << func_end << "\n";

        entryPcToReturnPcs[func_start].insert(func_end);
    }

    std::ifstream fff;
    s2eOpen(fff, "callerToFollowUp");

    while (fff.read((char*)&followUp, 4).read((char*)&caller, 4)) {
        errs() << "caller: " << intToHex(caller) << " follow_up: " << intToHex(followUp) << "\n";
        errs() << "caller: " << caller << " follow_up: " << followUp << "\n";
        assert(callerPcToFollowUpPc.insert({caller, followUp}).second && "caller is not unique");
    }


    for(std::map<uint32_t, std::set<uint32_t> >::iterator i = entryPcToCallerPcs.begin(), e = entryPcToCallerPcs.end(); i != e; ++i){
        std::ostringstream ss;
        ss << intToHex(i->first);
        std::string funcFile("func_" + ss.str());
        
        std::ifstream fff;
        if(!s2eOpen(fff, funcFile, false))
            continue;
        
        errs() << "found function: " << intToHex(i->first) << "\n";
        errs() << "found function: " << i->first << "\n";
        
        uint32_t bb_pc;
        while (fff.read((char*)&bb_pc, 4)) {
            errs() << "bb_pc: " << intToHex(bb_pc) << "\n";
            errs() << "bb_pc: " << bb_pc << "\n";
            entryPcToBBPcs[i->first].insert(bb_pc);
            BBPcToEntryPcs[bb_pc].insert(i->first);
        }
    }

/*
    std::ifstream wrapper;
    if(!s2eOpen(wrapper, "func_7b"))
        return;
    errs() << "found function: " << "7b" << "\n";
    errs() << "found function: " << 123 << "\n";
    uint32_t bb_pc;
    while (wrapper.read((char*)&bb_pc, 4)) {
        errs() << "bb_pc: " << intToHex(bb_pc) << "\n";
        errs() << "bb_pc: " << bb_pc << "\n";
        entryPcToBBPcs[123].insert(bb_pc);
        BBPcToEntryPc[bb_pc] = 123;
    }
*/
}

bool FixCFG::hasPc(BasicBlock *bb, GlobalVariable *PC, uint32_t pc) {
    for (Instruction &inst : *bb) {
        if (StoreInst *store = dyn_cast<StoreInst>(&inst)) {
            if (PC == store->getPointerOperand()) {
                if (ConstantInt *storePc = dyn_cast<ConstantInt>(store->getValueOperand())) {
                    if (storePc->getZExtValue() == pc) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}


void FixCFG::findCallerBBs(Function &F, std::unordered_map<unsigned, std::unordered_set<BasicBlock*> > &callerPcToCallerBB){
    GlobalVariable *PC = F.getParent()->getNamedGlobal("PC");        

    for(auto &p : callerPcToFollowUpPc){
        for(BasicBlock &B : F){
            if(!hasPc(&B, PC, p.first))
                continue;
            
            
            callerPcToCallerBB[p.first].insert(&B);
            //assert(callerPcToCallerBB.insert({p.first, &B}).second && "Duplicate BB for the same pc");
            /*
            if(callerPcToCallerBB.insert({p.first, &B}).second == false){
                DBG("Duplicate:");
                DBG("caller_pc: " << p.first);
                DBG(callerPcToCallerBB[p.first]->getName());
                DBG(B.getName());
            }
            */
        }
        //assert(callerPcToCallerBB.find(p.first) != callerPcToCallerBB.end() && "No BB for caller");
    }


    DBG("___CallerPcToCallerBB");
    for(auto &p : callerPcToCallerBB){
        for(auto *B : p.second){
            DBG("caller_pc: " << p.first << " callerBB: " << B->getName());
        }
    }
}

void FixCFG::getBBs(Function &F, std::map<unsigned, BasicBlock*> &BBMap){
    BasicBlock &entryBB = F.getEntryBlock();
    for(BasicBlock &B : F){
        if(!isRecoveredBlock(&B))
            continue;
        // Discard BBs that has no preds
        if(pred_begin(&B) == pred_end(&B) && &B != &entryBB){
            DBG("Discard BB: " << B.getName());
            continue;
        }
        unsigned pc = getBlockAddress(&B);
        BBMap[pc] = &B;
        DBG("BB pc: " << intToHex(pc) << ":" << pc << " BB: " << B.getName());
    }
}

//TODO: check BB_8049b70.from_BB_8049aa0 and BB_8049b70
void FixCFG::getBBsWithLibCalls(Function &F, std::unordered_set<BasicBlock*> &BBSet){
    BasicBlock &entryBB = F.getEntryBlock();
    for(BasicBlock &B : F){
        if(!isRecoveredBlock(&B))
            continue;
        // Discard BBs that has no preds
        if(pred_begin(&B) == pred_end(&B) && &B != &entryBB){
            DBG("Discard BB: " << B.getName());
            continue;
        }
        

        for(Instruction &I : B){
            if(!I.getMetadata("funcname"))
                continue;
            BBSet.insert(&B);
            DBG("BB with Func Call: " << B.getName());
            break;
        }
    }
}

void FixCFG::addSwitchCases(std::map<unsigned, BasicBlock*> &BBMap, 
                             std::unordered_map<BasicBlock *, std::unordered_set<unsigned> > &newSuccs){

    for(auto &p : newSuccs){
        for(auto S = succ_begin(p.first), E = succ_end(p.first); S != E; ++S){
            p.second.erase(getBlockAddress(*S));
        }

        if(p.second.empty()){
            continue;
        }

        for(unsigned succPc : p.second){
            BasicBlock *succBB = BBMap[succPc];
            SwitchInst *sw = dyn_cast<SwitchInst>(p.first->getTerminator());
            assert(sw && "Terminator is not switch inst");
            ConstantInt *addr = ConstantInt::get(
                        Type::getInt32Ty(succBB->getContext()), succPc);

            //add the case if doesn't exist
            if(sw->findCaseValue(addr) == sw->case_default()){
                //updateList.push_back({sw, nextBB});
                sw->addCase(addr, succBB);
                //DBG(*sw);
            }
        }
    }
}


bool FixCFG::runOnModule(Module &m) {
    
    loadFunctionMetadata();

    Function *wrapper = m.getFunction("wrapper");
    assert(wrapper && "No wrapper Function");

    std::unordered_set<BasicBlock*> BBSet;
    getBBsWithLibCalls(*wrapper, BBSet);
    
    std::map<unsigned, BasicBlock*> BBMap;
    getBBs(*wrapper, BBMap);

    std::unordered_map<unsigned, std::unordered_set<BasicBlock*> > callerPcToCallerBB;
    findCallerBBs(*wrapper, callerPcToCallerBB);
    
    std::list<std::pair<SwitchInst*, BasicBlock*>> updateList;
    std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *> > jmpBBs;
    //std::unordered_map<BasicBlock *, std::unordered_set<BasicBlock *> > callBBs;

    for(BasicBlock *B : BBSet){
        //find caller of this function
        unsigned pc = getBlockAddress(B);
        auto callerPcSet = entryPcToCallerPcs[pc];
        
        //check if each pred is callerBB
        //if it is, then it function called by call inst
        //otherwise, it is called by jmp
        for(auto P = pred_begin(B), E = pred_end(B); P != E; ++P){
            if(pred_begin(*P) == pred_end(*P) && &(wrapper->getEntryBlock()) != *P)
                continue;
            MDNode *md = getBlockMeta(*P, "inlined_lib_func");
            if (md){
                continue;
            }
                
            bool found = false;
            for(unsigned callerPc : callerPcSet){
                for(auto *callerBB : callerPcToCallerBB[callerPc]){
                    if(callerBB == *P){
                        //callBBs[B].insert(*P);
                        found = true;
                        break;
                    }
                }
                if(found) break;
            }
            if(!found && BBSet.find(*P) == BBSet.end()){
                //MDNode *md = getBlockMeta(*P, "inlined_lib_func");
                //if (!md){
                    jmpBBs[B].insert(*P);
                //}                
            }
        }
    }

    for(auto &p : jmpBBs){
        DBG("BB: " << p.first->getName());
        for(BasicBlock *B : p.second){
            DBG("Jump BB: " << B->getName());
        }
        DBG("-----------------");
    }
    
    std::unordered_map<BasicBlock *, std::unordered_set<unsigned> > newSuccs;

    //Add new succs for jmp blocks
    DBG("-----newSuccs-Jump------");
    for(auto &p : jmpBBs){
        DBG("BB: " << p.first->getName());
        for(BasicBlock *B : p.second){
            //func that called func(p.first) with jmp
            unsigned BPc = getBlockAddress(B);
            DBG("BB_pred_pc: " << intToHex(BPc));
            for(unsigned callerFuncPc : BBPcToEntryPcs[BPc]){
                DBG("caller func: " << intToHex(callerFuncPc));
                for(unsigned callerOfCallerFunc : entryPcToCallerPcs[callerFuncPc]){
                    unsigned followUp = callerPcToFollowUpPc[callerOfCallerFunc];
                    DBG("follow_up to be added: " << intToHex(followUp));
                    newSuccs[p.first].insert(followUp);
                }
            }
        }
        DBG("-----------------");
    }

    //Add new succs for call blocks
    DBG("-----newSuccs-Call------");
    for(BasicBlock *B : BBSet){
        DBG("BB: " << B->getName());
        //find caller of this function
        unsigned pc = getBlockAddress(B);
        auto callerPcSet = entryPcToCallerPcs[pc];
        
        //check if each pred is callerBB
        //if it is, then it function called by call inst
        //otherwise, it is called by jmp
        for(auto P = pred_begin(B), E = pred_end(B); P != E; ++P){
            if(pred_begin(*P) == pred_end(*P) && &(wrapper->getEntryBlock()) != *P)
                continue;
            MDNode *md = getBlockMeta(*P, "inlined_lib_func");
            if (md){
                continue;
            }
                
            bool found = false;
            for(unsigned callerPc : callerPcSet){
                for(auto *callerBB : callerPcToCallerBB[callerPc]){
                    if(callerBB == *P){
                        unsigned followUp = callerPcToFollowUpPc[callerPc];
                        DBG("BB_pred_pc: " << callerBB->getName());
                        DBG("caller_pc: " << callerPc << " followUp: " << intToHex(followUp));
                        newSuccs[B].insert(followUp);
                        found = true;
                        break;
                    }
                }
                if(found) break;
            }
        }
        DBG("-----------------");
    }

    for(auto &p : updateList){
        ConstantInt *addr = ConstantInt::get(Type::getInt32Ty(p.second->getContext()), getBlockAddress(p.second));

        p.first->addCase(addr, p.second);
    } 

    addSwitchCases(BBMap, newSuccs);

    return true;
}

