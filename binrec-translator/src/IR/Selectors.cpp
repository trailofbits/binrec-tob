#include "Selectors.h"

using namespace binrec;
using namespace llvm;

auto binrec::isLiftedFunction(const Function &f) -> bool {
    assert(f.hasName());
    StringRef name = f.getName();
    return name.startswith("Func_") && name != "Func_wrapper" && name != "Func_init";
}

LiftedFunctions::LiftedFunctions(Module &m) {
    c.reserve(m.size());
    for (Function &f : m) {
        if (isLiftedFunction(f)) {
            c.push_back(&f);
        }
    }
}

auto LiftedFunctions::size() const -> size_t { return c.size(); }

auto LiftedFunctions::begin() const -> iterator { return c.begin(); }

auto LiftedFunctions::end() const -> iterator { return c.end(); }

LocalRegisters::LocalRegisters(Function &f) {
    for (Instruction &i : f.getEntryBlock()) {
        if (auto *alloca = dyn_cast<AllocaInst>(&i)) {
            if (std::find(localRegisterNames.begin(), localRegisterNames.end(), alloca->getName()) !=
                localRegisterNames.end()) {
                c.push_back(alloca);
                if (alloca->getName() == "r_esp") {
                    knownRegisters[static_cast<int>(Register::ESP)] = alloca;
                } else if (alloca->getName() == "r_ebp") {
                    knownRegisters[static_cast<int>(Register::EBP)] = alloca;
                } else if (alloca->getName() == "r_eax") {
                    knownRegisters[static_cast<int>(Register::EAX)] = alloca;
                } else if (alloca->getName() == "r_edx") {
                    knownRegisters[static_cast<int>(Register::EDX)] = alloca;
                }
            }
        }
    }
}

auto LocalRegisters::begin() const -> iterator { return c.begin(); }
auto LocalRegisters::end() const -> iterator { return c.end(); }

auto LocalRegisters::find(Register reg) const -> AllocaInst * { return knownRegisters[static_cast<int>(reg)]; }
