#include "PassUtils.h"
#include "RegisterPasses.h"
#include <algorithm>
#include <fstream>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/BasicBlockUtils.h>

using namespace llvm;

namespace {
auto getSymbols(Module &m) -> std::map<int32_t, std::string> {
    std::map<int32_t, std::string> symmap;

    LLVMContext &ctx = m.getContext();
    std::ifstream f;
    fileOpen(f, "dynsym");
    std::string name;
    uint32_t addr = 0;

    while (f >> std::hex >> addr >> name) {
        symmap[addr] = name;
    }
    for (auto &it : symmap) {
        Constant *newsym = m.getOrInsertGlobal(it.second, Type::getInt32Ty(ctx));
        if (isa<GlobalVariable>(newsym)) {
            // gv->setExternallyInitialized(true);
            DBG("added dynamic symbol for: " << it.second);
        }
    }
    return symmap;
}

/// parse readelf -D -s to get the dynamic object symbols (not functions), then if we find one of those addresses as an
/// immmediate replace it with the symbol name
class ReplaceDynamicSymbolsPass : public PassInfoMixin<ReplaceDynamicSymbolsPass> {
public:
    auto run(Function &f, FunctionAnalysisManager &) -> PreservedAnalyses {
        auto symmap = getSymbols(*f.getParent());

        LLVMContext &ctx = f.getContext();
        Module *m = f.getParent();
        IRBuilder<> b(ctx);
        std::map<int32_t, std::string>::iterator lookup;
        std::list<CallInst *> loads;

        for (auto &bb : f) {
            for (auto &instruction : bb) {
                if (auto *call = dyn_cast<CallInst>(&instruction)) {
                    Function *f = call->getCalledFunction();
                    if (f && f->hasName()) {
                        StringRef name = f->getName();
                        if (name.endswith("_mmu") && name.startswith("__ld")) {
                            loads.push_back(call);
                        }
                    }
                }
            }
        }

        for (auto &it : loads) {
            CallInst *call = it;
            if (auto *addr = dyn_cast<ConstantInt>(call->getOperand(0))) {
                DBG("looking up addr " << addr->getZExtValue());
                lookup = symmap.find(addr->getZExtValue());
                if (lookup != symmap.end()) {
                    b.SetInsertPoint(call);
                    Instruction *sym = b.CreateLoad(m->getNamedGlobal(lookup->second));
                    ReplaceInstWithInst(call, sym);
                }
            }
        }

        return PreservedAnalyses::none();
    }
};
} // namespace

void binrec::addReplaceDynamicSymbolsPass(ModulePassManager &mpm) {
    mpm.addPass(createModuleToFunctionPassAdaptor(ReplaceDynamicSymbolsPass()));
}
