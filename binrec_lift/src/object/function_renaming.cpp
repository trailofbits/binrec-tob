#include "function_renaming.hpp"
#include "error.hpp"
#include "ir/selectors.hpp"

#define PASS_NAME "function_renaming"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace binrec;
using namespace llvm;
using namespace llvm::object;
using namespace std;

FunctionRenamingPass::FunctionRenamingPass()
{
    auto binary_or_err = createBinary("binary");
    if (auto err = binary_or_err.takeError()) {
        LLVM_ERROR(error) << err;
        throw binrec::lifting_error{"function_renaming", error};
    }
    binary = move(binary_or_err.get());
}

auto FunctionRenamingPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    auto *elf = dyn_cast<ELFObjectFileBase>(binary.getBinary());
    PASS_ASSERT(elf);

    DenseMap<uint64_t, Function *> address_to_func;
    for (Function &f : LiftedFunctions{m}) {
        StringRef name = f.getName();
        if (name == "Func_wrapper")
            continue;
        uint64_t address;
        PASS_ASSERT(!name.substr(5).consumeInteger(16, address));
        address_to_func.insert(make_pair(address, &f));
    }

    for (ELFSymbolRef symbol : elf->symbols()) {
        Expected<uint64_t> address = symbol.getAddress();
        if (address.takeError())
            continue;
        auto address_to_func_it = address_to_func.find(*address);
        if (address_to_func_it == address_to_func.end())
            continue;
        Expected<StringRef> name = symbol.getName();
        if (name.takeError())
            continue;
        address_to_func_it->second->setName("Func_" + *name);
    }
    return PreservedAnalyses::all();
}
