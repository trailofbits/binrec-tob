#ifndef BINREC_BYTE_UNIT_HPP
#define BINREC_BYTE_UNIT_HPP

#include <cstdint>
#include <nlohmann/json.hpp>
#include <sstream>

namespace binrec {
    template <typename Tag> struct ByteUnit {
        std::int64_t value{};

        ByteUnit() = default;
        explicit ByteUnit(std::int64_t value) : value(value) {}

        auto operator==(ByteUnit other) const -> bool
        {
            return value == other.value;
        }
        auto operator!=(ByteUnit other) const -> bool
        {
            return value != other.value;
        }
        auto operator<(ByteUnit other) const -> bool
        {
            return value < other.value;
        }
        auto operator<=(ByteUnit other) const -> bool
        {
            return value <= other.value;
        }
        auto operator>=(ByteUnit other) const -> bool
        {
            return value >= other.value;
        }
        auto operator>(ByteUnit other) const -> bool
        {
            return value > other.value;
        }

        auto operator+(ByteUnit other) const -> ByteUnit
        {
            return ByteUnit(value + other.value);
        }
    };

    // NOLINTNEXTLINE
    template <typename Tag> void to_json(nlohmann::json &j, ByteUnit<Tag> bytes)
    {
        j = bytes.value;
    }

    // NOLINTNEXTLINE
    template <typename Tag> void from_json(const nlohmann::json &j, ByteUnit<Tag> &bytes)
    {
        bytes.value = j;
    }

    struct BytesTag;
    using Bytes = ByteUnit<BytesTag>;

    inline auto operator<<(std::ostream &os, Bytes bytes) -> std::ostream &
    {
        os << bytes.value << " B";
        return os;
    }

} // namespace binrec

#endif
