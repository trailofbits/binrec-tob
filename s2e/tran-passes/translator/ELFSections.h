#ifndef _ELF_SECTIONS_H
#define _ELF_SECTIONS_H

#include <set>
#include "llvm/Pass.h"

using namespace llvm;

// names of sections to copy
std::set<std::string> COPY_SECTIONS = {".data", ".rodata", ".bss"};

struct ELFSections : public ModulePass {
    static char ID;
    ELFSections() : ModulePass(ID) {}

    virtual bool runOnModule(Module &m);
};

#endif  // _ELF_SECTIONS_H
