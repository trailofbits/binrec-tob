#include "RegisterPasses.h"

extern "C" LLVM_ATTRIBUTE_WEAK auto llvmGetPassPluginInfo() -> ::llvm::PassPluginLibraryInfo {
    return binrec::getBinRecPluginInfo();
}
