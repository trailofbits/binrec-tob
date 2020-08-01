#include "SegfaultDebug.h"
#include "PassUtils.h"
#include "DebugUtils.h"
#include <stdlib.h>
#include <stdio.h>

using namespace llvm;

char SegfaultDebug::ID = 0;
static RegisterPass<SegfaultDebug> X("segfault-debug",
        "Insert print statements to debug segfaults", false, false);

static int counter = 0;
bool SegfaultDebug::runOnFunction(Function &f) {

    if(f.hasName() && !(f.getName().startswith("wrapper") || f.getName().startswith("helper_flds")
        || f.getName().startswith("pack") || f.getName().startswith("floatx80") || f.getName().startswith("float32"))){
        return false;
    }

    for (Function::iterator bb = f.begin(), bbe = f.end(); bb != bbe; ++bb) {
    BasicBlock &b = *bb;
    for (BasicBlock::iterator i = b.begin(), ie = b.end(); i != ie; ++i) {
      if (ReturnInst *si = dyn_cast<ReturnInst>(&*i))
        continue;
      if (BranchInst *si = dyn_cast<BranchInst>(&*i))
        continue;
      if (PHINode *si = dyn_cast<PHINode>(&*i))
        continue;
      /*if (BranchInst *si = dyn_cast<BranchInst>(&*i)) {
        continue;
        IRBuilder<> Builder(&*i);
        char buf[64];
        sprintf(buf, "%d\n", counter++);
        std::string s(buf);
        buildWrite(Builder, s);
        //Value *StoreAddr = Builder.CreatePtrToInt(si->getPointerOperand(), Builder.getInt32Ty());
        //Value *Masked = Builder.CreateAnd(StoreAddr, 0xffff);
        //Value *AlignedAddr = Builder.CreateIntToPtr(Masked, si->getPointerOperand()->getType());
        // ...
      }*/
      if (isa<ICmpInst>(&*i) || isa<ExtractValueInst>(&*i) || isa<LoadInst>(&*i) || isa<StoreInst>(&*i) || isa<BitCastInst>(&*i) || isa<GetElementPtrInst>(&*i)) {
        IRBuilder<> Builder(&*i);
        //std::vector<Value*> operands;
        //DebugLoc DL = DebugLoc::get(counter++, 0, b.getTerminator()->getMetadata("succs"));
       // Instruction *I = &*i;
        //I->setDebugLoc(DL);
        char buf[64];
        sprintf(buf, "%d\n", counter++);
        std::string s(buf);
        buildWrite(Builder, s);

        //Value *StoreAddr = Builder.CreatePtrToInt(si->getPointerOperand(), Builder.getInt32Ty());
        //Value *Masked = Builder.CreateAnd(StoreAddr, 0xffff);
        //Value *AlignedAddr = Builder.CreateIntToPtr(Masked, si->getPointerOperand()->getType());
        // ...
      }
    }
  }
    return true;
}


