#include "binrec/tracing/trace_info.hpp"
#include <algorithm>
#include <iterator>
#include <nlohmann/json.hpp>

using namespace binrec;
using nlohmann::json;

namespace binrec {
    // NOLINTNEXTLINE
    void to_json(json &j, const MemoryAccess &ma)
    {
        j = json{
            {"pc", ma.pc},
            {"offset", ma.offset},
            {"isWrite", ma.isWrite},
            {"isLocalAccess", ma.isLocalAccess},
            {"size", ma.size},
            {"isDirect", ma.isDirect},
            {"fnBase", ma.fnBase}};
    }

    // NOLINTNEXTLINE
    void from_json(const json &j, MemoryAccess &ma)
    {
        j.at("pc").get_to(ma.pc);
        j.at("offset").get_to(ma.offset);
        j.at("isWrite").get_to(ma.isWrite);
        j.at("isLocalAccess").get_to(ma.isLocalAccess);
        j.at("size").get_to(ma.size);
        j.at("isDirect").get_to(ma.isDirect);
        j.at("fnBase").get_to(ma.fnBase);
    }

    // NOLINTNEXTLINE
    void to_json(json &j, const Successor &s)
    {
        j = json{{"pc", s.pc}, {"successor", s.successor}};
    }

    // NOLINTNEXTLINE
    void from_json(const json &j, Successor &s)
    {
        j.at("pc").get_to(s.pc);
        j.at("successor").get_to(s.successor);
    }

    // NOLINTNEXTLINE
    void to_json(json &j, const FunctionLog &fl)
    {
        j = json{
            {"entries", fl.entries},
            {"entryToCaller", fl.entryToCaller},
            {"entryToReturn", fl.entryToReturn},
            {"callerToFollowUp", fl.callerToFollowUp},
            {"entryToTbs", fl.entryToTbs}};
    }

    // NOLINTNEXTLINE
    void from_json(const json &j, FunctionLog &fl)
    {
        j.at("entries").get_to(fl.entries);
        j.at("entryToCaller").get_to(fl.entryToCaller);
        j.at("entryToReturn").get_to(fl.entryToReturn);
        j.at("callerToFollowUp").get_to(fl.callerToFollowUp);
        j.at("entryToTbs").get_to(fl.entryToTbs);
    }

    // NOLINTNEXTLINE
    void to_json(json &j, const TraceInfo &ti)
    {
        j = json{
            {"stackSizes", ti.stackFrameSizes},
            {"stackDifference", ti.stackDifference},
            {"memoryAccesses", ti.memoryAccesses},
            {"successors", ti.successors},
            {"functionLog", ti.functionLog}};
    }

    template <typename T> void maybeGetTo(const char *key, const json &j, T &o)
    {
        auto it = j.find(key);
        if (it != j.end()) {
            it->get_to(o);
        }
    }

    // NOLINTNEXTLINE
    void from_json(const json &j, TraceInfo &ti)
    {
        maybeGetTo("stackSizes", j, ti.stackFrameSizes);
        maybeGetTo("stackDifference", j, ti.stackDifference);
        maybeGetTo("memoryAccesses", j, ti.memoryAccesses);
        maybeGetTo("successors", j, ti.successors);
        maybeGetTo("functionLog", j, ti.functionLog);
    }
} // namespace binrec

namespace {
    std::weak_ptr<TraceInfo> ptr;
} // namespace

auto TraceInfo::get() -> std::shared_ptr<TraceInfo>
{
    std::shared_ptr<TraceInfo> sptr;
    if (ptr.expired()) {
        sptr = std::make_shared<TraceInfo>();
        ptr = sptr;
    } else {
        sptr = ptr.lock();
    }
    assert(sptr);
    return sptr;
}

// Create a deep copy of the current trace info
auto TraceInfo::getCopy() -> TraceInfo *
{
    TraceInfo *tiCopy = new TraceInfo();

    std::copy(
        stackFrameSizes.begin(),
        stackFrameSizes.end(),
        std::inserter(tiCopy->stackFrameSizes, tiCopy->stackFrameSizes.begin()));
    std::copy(
        stackDifference.begin(),
        stackDifference.end(),
        std::inserter(tiCopy->stackDifference, tiCopy->stackDifference.begin()));
    std::copy(
        memoryAccesses.begin(),
        memoryAccesses.end(),
        std::inserter(tiCopy->memoryAccesses, tiCopy->memoryAccesses.begin()));
    std::copy(
        successors.begin(),
        successors.end(),
        std::inserter(tiCopy->successors, tiCopy->successors.begin()));
    std::copy(
        functionLog.entries.begin(),
        functionLog.entries.end(),
        std::inserter(tiCopy->functionLog.entries, tiCopy->functionLog.entries.begin()));
    std::copy(
        functionLog.entryToCaller.begin(),
        functionLog.entryToCaller.end(),
        std::inserter(
            tiCopy->functionLog.entryToCaller,
            tiCopy->functionLog.entryToCaller.begin()));
    std::copy(
        functionLog.entryToReturn.begin(),
        functionLog.entryToReturn.end(),
        std::inserter(
            tiCopy->functionLog.entryToReturn,
            tiCopy->functionLog.entryToReturn.begin()));
    std::copy(
        functionLog.callerToFollowUp.begin(),
        functionLog.callerToFollowUp.end(),
        std::inserter(
            tiCopy->functionLog.callerToFollowUp,
            tiCopy->functionLog.callerToFollowUp.begin()));
    std::copy(
        functionLog.entryToTbs.begin(),
        functionLog.entryToTbs.end(),
        std::inserter(tiCopy->functionLog.entryToTbs, tiCopy->functionLog.entryToTbs.begin()));

    return tiCopy;
}

void TraceInfo::restoreFromCopy(TraceInfo *copyTi)
{
    stackFrameSizes.clear();
    stackDifference.clear();
    memoryAccesses.clear();
    successors.clear();
    functionLog.entries.clear();
    functionLog.entryToCaller.clear();
    functionLog.entryToReturn.clear();
    functionLog.callerToFollowUp.clear();
    functionLog.entryToTbs.clear();

    add(*copyTi);

    return;
}

void TraceInfo::add(const TraceInfo &ti)
{
    for (auto &[function, size] : ti.stackFrameSizes) {
        auto it = stackFrameSizes.find(function);
        if (it == stackFrameSizes.end()) {
            stackFrameSizes.emplace(function, size);
        } else {
            it->second = std::max(it->second, size);
        }
    }

    for (auto &[function, difference] : ti.stackDifference) {
        auto it = stackDifference.find(function);
        if (it == stackDifference.end()) {
            stackDifference.emplace(function, difference);
        } else {
            assert(it->second == difference);
        }
    }

    std::copy(
        ti.memoryAccesses.begin(),
        ti.memoryAccesses.end(),
        std::back_inserter(memoryAccesses));
    std::copy(
        ti.successors.begin(),
        ti.successors.end(),
        std::inserter(successors, successors.begin()));

    if (functionLog.entries.empty()) {
        std::copy(
            ti.functionLog.entries.begin(),
            ti.functionLog.entries.end(),
            std::back_inserter(functionLog.entries));
    }
    std::copy(
        ti.functionLog.entryToCaller.begin(),
        ti.functionLog.entryToCaller.end(),
        std::inserter(functionLog.entryToCaller, functionLog.entryToCaller.begin()));
    std::copy(
        ti.functionLog.entryToReturn.begin(),
        ti.functionLog.entryToReturn.end(),
        std::inserter(functionLog.entryToReturn, functionLog.entryToReturn.begin()));
    std::copy(
        ti.functionLog.callerToFollowUp.begin(),
        ti.functionLog.callerToFollowUp.end(),
        std::inserter(functionLog.callerToFollowUp, functionLog.callerToFollowUp.begin()));
    for (auto &entry : ti.functionLog.entryToTbs) {
        auto targetEntry = functionLog.entryToTbs.insert({entry.first, {}});
        std::copy(
            entry.second.begin(),
            entry.second.end(),
            std::inserter(targetEntry.first->second, targetEntry.first->second.begin()));
    }
}

auto Successor::operator<(const Successor &other) const -> bool
{
    if (pc == other.pc) {
        return successor < other.successor;
    }
    return pc < other.pc;
}

auto binrec::operator<<(std::ostream &os, const TraceInfo &ti) -> std::ostream &
{
    json j = ti;
    os << j;
    return os;
}

auto binrec::operator>>(std::istream &is, TraceInfo &ti) -> std::istream &
{
    json j;
    is >> j;
    ti = j.get<TraceInfo>();
    return is;
}
