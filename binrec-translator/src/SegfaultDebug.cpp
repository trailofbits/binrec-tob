#include "SegfaultDebug.h"
#include "DebugUtils.h"
#include "PassUtils.h"
#include <cstdio>
#include <cstdlib>

using namespace llvm;

char SegfaultDebug::ID = 0;
static RegisterPass<SegfaultDebug> X("segfault-debug", "Insert print statements to debug segfaults", false, false);

static int counter = 0;
auto SegfaultDebug::runOnFunction(Function &f) -> bool {

    if (f.hasName() &&
        !(f.getName().startswith("Func_wrapper") || f.getName().startswith("helper_flds") ||
          f.getName().startswith("pack") || f.getName().startswith("floatx80") || f.getName().startswith("float32"))) {
        return false;
    }

    for (auto &b : f) {
        for (auto &i : b) {
            if (isa<ReturnInst>(&i))
                continue;
            if (isa<BranchInst>(&i))
                continue;
            if (isa<PHINode>(&i))
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
            if (isa<ICmpInst>(&i) || isa<ExtractValueInst>(&i) || isa<LoadInst>(&i) || isa<StoreInst>(&i) ||
                isa<BitCastInst>(&i) || isa<GetElementPtrInst>(&i)) {
                IRBuilder<> Builder(&i);
                // std::vector<Value*> operands;
                // DebugLoc DL = DebugLoc::get(counter++, 0, b.getTerminator()->getMetadata("succs"));
                // Instruction *I = &*i;
                // I->setDebugLoc(DL);
                char buf[64];
                sprintf(buf, "%d\n", counter++);
                std::string s(buf);
                buildWrite(Builder, s);

                // Value *StoreAddr = Builder.CreatePtrToInt(si->getPointerOperand(), Builder.getInt32Ty());
                // Value *Masked = Builder.CreateAnd(StoreAddr, 0xffff);
                // Value *AlignedAddr = Builder.CreateIntToPtr(Masked, si->getPointerOperand()->getType());
                // ...
            }
        }
    }
    return true;
}
