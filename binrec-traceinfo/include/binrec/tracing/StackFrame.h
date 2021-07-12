#ifndef BINREC_STACKFRAME_H
#define BINREC_STACKFRAME_H

#include "binrec/Address.h"
#include "binrec/ByteUnit.h"
#include <nlohmann/json.hpp>
#include <set>
#include <utility>
#include <vector>

namespace binrec {
class StackFrame {
public:
    struct Id {
        FunctionAddress functionAddress;
        InstructionAddress callerAddress;

        auto operator<(Id other) const -> bool;
    };

    using FrameSizeUpdate = std::pair<InstructionAddress, Bytes>;

    struct Data {
        Id id;
        std::vector<FrameSizeUpdate> spUpdates;
        std::vector<FrameSizeUpdate> fpUpdates;
    };

private:
    using FrameSizeUpdates = std::set<FrameSizeUpdate>;

public:
    const FunctionAddress functionAddress;
    const InstructionAddress callerAddress;

private:
    FrameSizeUpdates spUpdates;
    FrameSizeUpdates fpUpdates;

public:
    StackFrame(FunctionAddress functionAddress, InstructionAddress callerAddress);
    explicit StackFrame(const Data &data);

    [[nodiscard]] auto id() const -> Id;
    [[nodiscard]] auto asData() const -> Data;

    void recordStackPointerUpdate(InstructionAddress at, Bytes offset);
    void recordFramePointerUpdate(InstructionAddress at, Bytes offset);
};

void to_json(nlohmann::json &j, const StackFrame::Data &frame);   // NOLINT
void from_json(const nlohmann::json &j, StackFrame::Data &frame); // NOLINT
} // namespace binrec

#endif
