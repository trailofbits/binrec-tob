#ifndef BINREC_SELECTORS_HPP
#define BINREC_SELECTORS_HPP

#include "register.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>

namespace binrec {
    [[nodiscard]] auto is_lifted_function(const llvm::Function &f) -> bool;

    class LiftedFunctions {
        using Container = std::vector<llvm::Function *>;

    public:
        using iterator = llvm::pointee_iterator<Container::const_iterator>;

    private:
        Container c;

    public:
        explicit LiftedFunctions(llvm::Module &m);

        [[nodiscard]] auto size() const -> size_t;

        [[nodiscard]] auto begin() const -> iterator;
        [[nodiscard]] auto end() const -> iterator;
    };

    class LocalRegisters {
        using Container = std::vector<llvm::AllocaInst *>;

    public:
        using iterator = llvm::pointee_iterator<Container::const_iterator>;

    private:
        Container c;
        std::array<llvm::AllocaInst *, 4> known_registers{};

    public:
        explicit LocalRegisters(llvm::Function &f);

        [[nodiscard]] auto begin() const -> iterator;
        [[nodiscard]] auto end() const -> iterator;

        [[nodiscard]] auto find(Register reg) const -> llvm::AllocaInst *;
    };
} // namespace binrec

#endif
