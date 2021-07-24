#include "binrec/tracing/stack_frame.hpp"
#include <nlohmann/json.hpp>

using namespace binrec;
using nlohmann::json;

StackFrame::StackFrame(FunctionAddress functionAddress, InstructionAddress callerAddress) :
        functionAddress{functionAddress},
        callerAddress{callerAddress}
{
}

StackFrame::StackFrame(const Data &data) :
        functionAddress{data.id.functionAddress},
        callerAddress{data.id.callerAddress},
        spUpdates{data.spUpdates.begin(), data.spUpdates.end()},
        fpUpdates{data.fpUpdates.begin(), data.fpUpdates.end()}
{
}

auto StackFrame::id() const -> StackFrame::Id
{
    return {functionAddress, callerAddress};
}

auto StackFrame::asData() const -> StackFrame::Data
{
    // TODO: CLion shows wrong parameter hints. Report bug?
    return {id(), {spUpdates.begin(), spUpdates.end()}, {fpUpdates.begin(), fpUpdates.end()}};
}

void StackFrame::recordStackPointerUpdate(InstructionAddress at, Bytes value)
{
    spUpdates.emplace(at, value);
}

void StackFrame::recordFramePointerUpdate(InstructionAddress at, Bytes value)
{
    fpUpdates.emplace(at, value);
}

auto StackFrame::Id::operator<(StackFrame::Id other) const -> bool
{
    if (functionAddress < other.functionAddress) {
        return true;
    }
    if (functionAddress > other.functionAddress) {
        return false;
    }
    return callerAddress < other.callerAddress;
}

namespace binrec {
    // NOLINTNEXTLINE
    void to_json(json &j, StackFrame::Id id)
    {
        j = json{id.functionAddress, id.callerAddress};
    }

    // NOLINTNEXTLINE
    void from_json(const json &j, StackFrame::Id &id)
    {
        j[0].get_to(id.functionAddress);
        j[1].get_to(id.callerAddress);
    }

    void to_json(json &j, const StackFrame::Data &frame)
    {
        j = json{{"id", frame.id}, {"sp", frame.spUpdates}, {"fp", frame.fpUpdates}};
    }

    void from_json(const json &j, StackFrame::Data &frame)
    {
        j["id"].get_to(frame.id);
        j["sp"].get_to(frame.spUpdates);
        j["fp"].get_to(frame.fpUpdates);
    }
} // namespace binrec
