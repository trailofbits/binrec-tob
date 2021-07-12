#include "ReplaceLocalFunctionPointers.h"
#include "MetaUtils.h"
#include "PassUtils.h"
#include <algorithm>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

using namespace llvm;

char ReplaceLocalFunctionPointers::ID = 0;
static RegisterPass<ReplaceLocalFunctionPointers>
    X("replace-local-function-pointers",
      "Determine original address of local functions, determine corresponding lifted local functions, \
        replace usages of original with lifted version",
      false, false);

auto ReplaceLocalFunctionPointers::doInitialization(Module &m) -> bool {
    DBG("called rlfp initialize \n");
    for (auto &f : m) {
        for (auto &bb : f) {
            if (isRecoveredBlock(&bb)) {
                DBG("inserting block: " << getBlockAddress(&bb));
                allOriginalFunctions[getBlockAddress(&bb)] = &bb;
            }
        }
    }
    return true;
}

auto ReplaceLocalFunctionPointers::runOnFunction(Function &f) -> bool {
    // DBG("called rlfp \n");
    std::map<unsigned, BasicBlock *>::iterator lookup;
    // for (auto &bb : f){
    // for (auto &inst : bb){
    // if (!isa<SwitchInst>(&inst)){
    // for (auto &use : inst.operands()){
    // if (ConstantInt *ciop = dyn_cast<ConstantInt>(use)) {
    // lookup = allOriginalFunctions.find(ciop->getZExtValue());
    // if (lookup  != allOriginalFunctions.end()){
    ////if (std::map<>::iterator lookup = allOriginalFunctions.find(ciop->getZExtValue()) !=
    /// allOriginalFunctions.end()){
    // DBG("Type of inst : "<<inst.getOpcode());
    // inst.print(llvm::errs());
    // Constant *blam =  BlockAddress::get(lookup->second);
    ////blam->getType()->print(llvm::errs());
    // use = blam;
    // use->print(llvm::errs());
    //}
    //}
    //}
    //}
    //}
    //}
    LLVMContext &ctx = f.getContext();
    IRBuilder<> b(ctx);
    // std::map<int32_t,std::string>::iterator lookup;
    std::list<CallInst *> stores;
    std::list<CallInst *> tempStores;
    DBG("---------------ALL STORES----------------");
    for (auto &bb : f) {
        for (auto &instruction : bb) { // code assumes instructions visited in order
            if (auto *call = dyn_cast<CallInst>(&instruction)) {
                Function *f = call->getCalledFunction();
                if (f && f->hasName()) {
                    StringRef name = f->getName();
                    if (name.endswith("_mmu") && name.startswith("__st")) {
                        tempStores.push_back(call);
                    }
                }
            }
        }
        if (!tempStores.empty()) {
            tempStores.pop_back();
            stores.splice(stores.end(), tempStores);
        }
    }

    DBG("---------------ALL STORES----------------");

    DBG("---------------FUNC PTR STORES----------------");
    for (auto &call : stores) {
        // call->print(llvm::errs());
        for (auto &use : call->operands()) {
            if (auto *ciop = dyn_cast<ConstantInt>(use)) {
                lookup = allOriginalFunctions.find(ciop->getZExtValue());
                if (lookup != allOriginalFunctions.end()) {
                    // if (std::map<>::iterator lookup = allOriginalFunctions.find(ciop->getZExtValue()) !=
                    // allOriginalFunctions.end()){
                    call->print(llvm::errs());

                    // Constant *blam =  BlockAddress::get(lookup->second);
                    b.SetInsertPoint(call);
                    Value *sym =
                        b.CreateBitOrPointerCast(BlockAddress::get(lookup->second), IntegerType::getInt32Ty(ctx));
                    use = sym;
                    // ReplaceInstWithInst(call, sym);

                    call->print(llvm::errs());
                    DBG("");
                }
            }
        }

        // if (ConstantInt *addr = dyn_cast<ConstantInt>(call->getOperand(0))) {
        ////DBG("replacing load to "<< addr->getZExtValue() <<" with load of "<<lookup->second);
        // DBG("looking up addr "<< addr->getZExtValue());
        // lookup = symmap.find(addr->getZExtValue());
        // if(lookup != symmap.end()){
        ////WARNING("found a match in symmap");
        // b.SetInsertPoint(call);
        // Instruction *sym = b.CreateLoad(m->getNamedGlobal(lookup->second));
        // ReplaceInstWithInst(call, sym);
        //}
    }
    // DBG("finished replace dynamic symbols");
    return true;
}
