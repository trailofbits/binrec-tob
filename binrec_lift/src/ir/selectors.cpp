#include "selectors.hpp"

using namespace binrec;
using namespace llvm;
using namespace std;

auto binrec::is_lifted_function(const Function &f) -> bool
{
    assert(f.hasName());
    StringRef name = f.getName();
    return name.startswith("Func_") && name != "Func_wrapper" && name != "Func_init";
}

LiftedFunctions::LiftedFunctions(Module &m)
{
    c.reserve(m.size());
    for (Function &f : m) {
        if (is_lifted_function(f)) {
            c.push_back(&f);
        }
    }
}

auto LiftedFunctions::size() const -> size_t
{
    return c.size();
}

auto LiftedFunctions::begin() const -> iterator
{
    return c.begin();
}

auto LiftedFunctions::end() const -> iterator
{
    return c.end();
}

LocalRegisters::LocalRegisters(Function &f)
{
    for (Instruction &i : f.getEntryBlock()) {
        if (auto *alloca = dyn_cast<AllocaInst>(&i)) {
            if (std::find(
                    Local_Register_Names.begin(),
                    Local_Register_Names.end(),
                    alloca->getName()) != Local_Register_Names.end())
            {
                c.push_back(alloca);
                if (alloca->getName() == "r_esp") {
                    known_registers[static_cast<int>(Register::ESP)] = alloca;
                } else if (alloca->getName() == "r_ebp") {
                    known_registers[static_cast<int>(Register::EBP)] = alloca;
                } else if (alloca->getName() == "r_eax") {
                    known_registers[static_cast<int>(Register::EAX)] = alloca;
                } else if (alloca->getName() == "r_edx") {
                    known_registers[static_cast<int>(Register::EDX)] = alloca;
                }
            }
        }
    }
}

auto LocalRegisters::begin() const -> iterator
{
    return c.begin();
}
auto LocalRegisters::end() const -> iterator
{
    return c.end();
}

auto LocalRegisters::find(Register reg) const -> AllocaInst *
{
    return known_registers[static_cast<int>(reg)];
}
