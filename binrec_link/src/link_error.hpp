#ifndef BINREC_LINK_ERROR_HPP
#define BINREC_LINK_ERROR_HPP

#include <system_error>
#include <type_traits>

namespace binrec {
    enum class LinkError { Bad_Elf = 1, CC_Err };
    auto make_error_code(LinkError e) noexcept -> std::error_code;
} // namespace binrec

namespace std {
    template <> struct is_error_code_enum<binrec::LinkError> : true_type {
    };
} // namespace std

#endif
