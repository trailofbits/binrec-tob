//===-- Passes.h ------------------------------------------------*- C++ -*-===//
//
//                     The KLEE Symbolic Virtual Machine
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef INTRINSIC_CLEANER_H
#define INTRINSIC_CLEANER_H

#include "llvm/IR/Constants.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/CodeGen/IntrinsicLowering.h"

/*namespace llvm {
    class Function;
    class Instruction;
    class Module;
    class DataLayout;
    class Type;
}*/


// This is a module pass because it can add and delete module
// variables (via intrinsic lowering).
class IntrinsicCleanerPass : public llvm::ModulePass {
   // const llvm::DataLayout &DataLayout;
    llvm::IntrinsicLowering *IL;
    bool LowerIntrinsics;

    bool runOnBasicBlock(llvm::BasicBlock &b, const llvm::DataLayout &DataLayout);

    void injectIntrinsicAddImplementation(llvm::Module &M, const std::string &name, unsigned bits);
    void replaceIntrinsicAdd(llvm::Module &M, llvm::CallInst *CI);

public:
    static char ID;
    /*IntrinsicCleanerPass(const llvm::DataLayout &TD,
                         bool LI=true)
                             : llvm::ModulePass(ID),
                             DataLayout(TD),
                             IL(new llvm::IntrinsicLowering(TD)),
                             LowerIntrinsics(LI) {}
*/  IntrinsicCleanerPass(bool LI=true): llvm::ModulePass(ID), LowerIntrinsics(LI) {}
    ~IntrinsicCleanerPass() { delete IL; }

    virtual bool runOnModule(llvm::Module &M);
    //virtual bool runOnFunction(llvm::Function &F);
};

#endif
