#ifndef BINREC_ADDRESS_HPP
#define BINREC_ADDRESS_HPP

#include "byte_unit.hpp"
#include <sstream>

namespace binrec {
    template <typename Tag> struct TypedAddress {
        uint64_t value{};

        TypedAddress() = default;
        explicit TypedAddress(uint64_t value) : value(value) {}

        auto operator==(TypedAddress other) const -> bool
        {
            return value == other.value;
        }
        auto operator!=(TypedAddress other) const -> bool
        {
            return value != other.value;
        }
        auto operator<(TypedAddress other) const -> bool
        {
            return value < other.value;
        }
        auto operator<=(TypedAddress other) const -> bool
        {
            return value <= other.value;
        }
        auto operator>=(TypedAddress other) const -> bool
        {
            return value >= other.value;
        }
        auto operator>(TypedAddress other) const -> bool
        {
            return value > other.value;
        }

        auto operator-(TypedAddress other) const -> Bytes
        {
            uint64_t diff;
            bool negate;
            if (value >= other.value) {
                diff = value - other.value;
                negate = false;
            } else {
                diff = other.value - value;
                negate = true;
            }
            assert(diff <= INT64_MAX);
            int64_t result = negate ? -diff : diff;
            return Bytes(result);
        }
    };

    template <typename Tag>
    auto operator<<(std::ostream &os, TypedAddress<Tag> address) -> std::ostream &
    {
        std::stringstream ss;
        ss << std::hex << "0x" << address.value;
        os << ss.str();
        return os;
    }

    // NOLINTNEXTLINE
    template <typename Tag> void to_json(nlohmann::json &j, TypedAddress<Tag> address)
    {
        j = address.value;
    }

    // NOLINTNEXTLINE
    template <typename Tag> void from_json(const nlohmann::json &j, TypedAddress<Tag> &address)
    {
        address.value = j;
    }

    struct FunctionAddressTag;
    using FunctionAddress = TypedAddress<FunctionAddressTag>;

    struct InstructionAddressTag;
    using InstructionAddress = TypedAddress<InstructionAddressTag>;

    struct StackAddressTag;
    using StackAddress = TypedAddress<StackAddressTag>;

    template <typename Address> class AddressRange {
        Address begin;
        Address end;

    public:
        AddressRange() : begin(), end() {}
        /// Create a closed ranged at a given address.
        explicit AddressRange(Address address) : begin(address), end(address) {}
        AddressRange(Address begin, Address end)
        {
            if (begin < end) {
                this->begin = begin;
                this->end = end;
            } else {
                this->begin = end;
                this->end = begin;
            }
        }

        auto contains(Address address) const -> bool
        {
            return begin <= address && address < end;
        }
        auto overlaps(AddressRange other) const -> bool
        {
            return end <= other.begin || other.end <= begin;
        }

        auto operator<(AddressRange other) const -> bool
        {
            return end <= other.begin;
        }
        auto operator>(AddressRange other) const -> bool
        {
            return begin >= other.end;
        }

        auto operator<(Address address) const -> bool
        {
            return end <= address;
        }
        auto operator>(Address address) const -> bool
        {
            return begin > address;
        }
    };

    using StackFrameRange = AddressRange<StackAddress>;
} // namespace binrec

#endif
