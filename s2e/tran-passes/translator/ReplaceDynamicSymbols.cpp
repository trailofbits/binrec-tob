#include <iostream>
#include <algorithm>

#include "ReplaceDynamicSymbols.h"
#include "PassUtils.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/BasicBlock.h"
using namespace llvm;

char ReplaceDynamicSymbols::ID = 0;
static RegisterPass<ReplaceDynamicSymbols> X("replace-dynamic-symbols",
        "parse readelf -D -s to get the dynamic object symbols (not functions), then if we find one of those \
        addresses as an immmediate replace it with the symbol name",
        false, false);

bool ReplaceDynamicSymbols::doInitialization(Module &m) {
    LLVMContext &ctx = m.getContext();
    std::ifstream f;
    fileOpen(f, s2eOutDir + "/dynsym");
    std::string name;
    uint32_t addr;

    while (f >> std::hex >> addr >> name ) {
        symmap[addr] = name;
    }
    for (auto it: symmap){
        Constant *newsym = m.getOrInsertGlobal(it.second,Type::getInt32Ty(ctx));
        ifcast(GlobalVariable, gv, newsym) {
            //gv->setExternallyInitialized(true);
            DBG("added dynamic symbol for: "<< it.second );
        }
    }
    return true;
}

bool ReplaceDynamicSymbols::runOnFunction(Function &f) {
    LLVMContext &ctx = f.getContext();
    Module *m = f.getParent();
    IRBuilder<> b(ctx);
    std::map<int32_t,std::string>::iterator lookup;
    std::list<CallInst*> loads;

    for (auto &bb : f){
        foreach(instruction , bb){
            if (CallInst *call = dyn_cast<CallInst>(instruction)) {
                Function *f = call->getCalledFunction();
                if (f && f->hasName()) {
                    StringRef name = f->getName();
                    if (name.endswith("_mmu") && name.startswith("__ld")){
                        loads.push_back(call);
                    }
                }
            }
        }
    }

    foreach(it, loads){
        CallInst *call = *it;
            //call->print(llvm::errs());
        ifcast(ConstantInt, addr, call->getOperand(0)){
                //DBG("replacing load to "<< addr->getZExtValue() <<" with load of "<<lookup->second);
                DBG("looking up addr "<< addr->getZExtValue());
            lookup = symmap.find(addr->getZExtValue());
            if(lookup != symmap.end()){
                //WARNING("found a match in symmap");
                b.SetInsertPoint(call);
                Instruction *sym = b.CreateLoad(m->getNamedGlobal(lookup->second));
                ReplaceInstWithInst(call, sym);
            }
        }
    }
    //DBG("finished replace dynamic symbols");
    return true;
}
