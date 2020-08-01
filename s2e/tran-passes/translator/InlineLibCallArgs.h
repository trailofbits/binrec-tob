#ifndef _INLINELIBCALLARGS_H
#define _INLINELIBCALLARGS_H

#include <map>

#include "llvm/Pass.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/IRBuilder.h"

using namespace llvm;

struct Signature {
    std::string name;
    bool isfloat;
    size_t retsize;
    std::vector<size_t> argsizes;
    Function *stub;
};

static inline std::string stubName(const std::string base) {
    return "__stub_" + base;
}

struct InlineLibCallArgs : public ModulePass {
    static char ID;
    InlineLibCallArgs() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);

private:
    Module *module;
    std::map<std::string, Signature> sigmap;

    void readLibcArgSizes();

    Function *getOrInsertStub(Signature &sig);
    void replaceCall(Module &m, CallInst *call, Signature &sig);
};

#endif  // _INLINELIBCALLARGS_H
