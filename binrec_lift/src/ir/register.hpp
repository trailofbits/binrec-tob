#ifndef BINREC_REGISTER_HPP
#define BINREC_REGISTER_HPP

#include <array>
#include <llvm/ADT/StringRef.h>

namespace binrec {
    constexpr std::array<llvm::StringRef, 12> Global_Trivial_Register_Names = {
        "PC",
        "R_EAX",
        "R_EBX",
        "R_ECX",
        "R_EDX",
        "R_EDI",
        "R_ESI",
        "R_EBP",
        "R_ESP",
        "cc_op",
        "cc_src",
        "cc_dst"};
    constexpr std::array<llvm::StringRef, 16> Global_Emulation_Var_Names = {
        "PC",
        "R_EAX",
        "R_EBX",
        "R_ECX",
        "R_EDX",
        "R_EDI",
        "R_ESI",
        "R_EBP",
        "R_ESP",
        "cc_op",
        "cc_src",
        "cc_dst",
        "fpus",
        "fpstt",
        "fpuc",
        "fpregs"};
    constexpr std::array<llvm::StringRef, 12> Local_Register_Names = {
        "pc",
        "r_eax",
        "r_ebx",
        "r_ecx",
        "r_edx",
        "r_edi",
        "r_esi",
        "r_ebp",
        "r_esp",
        "cc_op",
        "cc_src",
        "cc_dst"};

    enum class Register {
        EAX,
        EDX,
        ESP,
        EBP,
    };
} // namespace binrec

#endif
