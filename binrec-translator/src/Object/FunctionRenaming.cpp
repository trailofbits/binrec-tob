#include "FunctionRenaming.h"
#include "IR/Selectors.h"

using namespace binrec;
using namespace llvm;
using namespace llvm::object;
using namespace std;

FunctionRenamingPass::FunctionRenamingPass() {
    auto binaryOrErr = createBinary("binary");
    if (auto err = binaryOrErr.takeError()) {
        errs() << err << '\n';
        exit(-1);
    }
    binary = std::move(binaryOrErr.get());
}

auto FunctionRenamingPass::run(Module &m, ModuleAnalysisManager &mam) -> PreservedAnalyses {
    auto *elf = dyn_cast<ELFObjectFileBase>(binary.getBinary());
    assert(elf);

    DenseMap<uint64_t, Function *> addressToFunc;
    for (Function &f : LiftedFunctions{m}) {
        StringRef name = f.getName();
        if (name == "Func_wrapper")
            continue;
        uint64_t address;
        assert(!name.substr(5).consumeInteger(16, address));
        addressToFunc.insert(make_pair(address, &f));
    }

    for (ELFSymbolRef symbol : elf->symbols()) {
        Expected<uint64_t> address = symbol.getAddress();
        if (address.takeError())
            continue;
        auto addressToFuncIt = addressToFunc.find(*address);
        if (addressToFuncIt == addressToFunc.end())
            continue;
        Expected<StringRef> name = symbol.getName();
        if (name.takeError())
            continue;
        addressToFuncIt->second->setName("Func_" + *name);
    }
    return PreservedAnalyses::all();
}
