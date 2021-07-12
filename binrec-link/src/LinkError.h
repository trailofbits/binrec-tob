#ifndef BINREC_LINKERROR_H
#define BINREC_LINKERROR_H

#include <system_error>
#include <type_traits>

namespace binrec {
enum class LinkError { BadElf = 1, CCErr };

auto make_error_code(LinkError e) noexcept -> std::error_code;
} // namespace binrec

namespace std {
template <> struct is_error_code_enum<binrec::LinkError> : true_type {};
} // namespace std

#endif
