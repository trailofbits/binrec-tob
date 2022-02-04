#include "replace_dynamic_symbols.hpp"
#include "error.hpp"
#include "pass_utils.hpp"
#include <llvm/IR/IRBuilder.h>
#include <llvm/Object/ELFObjectFile.h>

using namespace binrec;
using namespace llvm;
using namespace llvm::object;
using namespace std;

static auto get_symbols(Module &m) -> map<uint32_t, string>
{
    auto binary_or_err = createBinary("binary");
    if (auto err = binary_or_err.takeError()) {
        LLVM_ERROR(error) << err;
        throw std::runtime_error{error};
    }
    auto &binary = binary_or_err.get();

    map<uint32_t, string> symmap;

    auto *elf = dyn_cast<ELFObjectFileBase>(binary.getBinary());
    for (ELFSymbolRef symbol : elf->getDynamicSymbolIterators()) {
        auto address = symbol.getAddress();
        if (address.takeError() || *address == 0)
            continue;
        auto type = symbol.getType();
        if (type.takeError() || *type == SymbolRef::ST_Function)
            continue;
        auto name_err = symbol.getName();
        if (name_err.takeError())
            continue;
        StringRef name = *name_err;
        if (name.contains("@")) {
            name = name.substr(0, name.find_first_of("@"));
        }
        symmap.emplace(*address, name);
    }


    LLVMContext &ctx = m.getContext();

    for (auto &it : symmap) {
        m.getOrInsertGlobal(it.second, Type::getInt32Ty(ctx));
    }

    return symmap;
}

// NOLINTNEXTLINE
auto ReplaceDynamicSymbolsPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    auto symbol_map = get_symbols(m);

    vector<CallInst *> loads;
    for (auto &f : m) {
        for (auto &bb : f) {
            for (auto &instruction : bb) {
                if (auto *call = dyn_cast<CallInst>(&instruction)) {
                    Function *called_function = call->getCalledFunction();
                    if (called_function && called_function->hasName()) {
                        StringRef name = called_function->getName();
                        if (name.endswith("_mmu") && name.startswith("__ld")) {
                            loads.push_back(call);
                        }
                    }
                }
            }
        }
    }

    for (auto &it : loads) {
        CallInst *call = it;
        if (auto *addr = dyn_cast<ConstantInt>(call->getOperand(0))) {
            DBG("looking up addr " << addr->getZExtValue());
            auto lookup = symbol_map.find(addr->getZExtValue());
            if (lookup != symbol_map.end()) {
                IRBuilder<> irb{call};
                Instruction *sym = irb.CreateLoad(m.getNamedGlobal(lookup->second));
                call->replaceAllUsesWith(sym);
                call->eraseFromParent();
            }
        }
    }

    return PreservedAnalyses::allInSet<CFGAnalyses>();
}
