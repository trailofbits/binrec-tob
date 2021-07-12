#ifndef BINREC_TRACEINFO_H
#define BINREC_TRACEINFO_H

#include <cstdint>
#include <istream>
#include <map>
#include <memory>
#include <ostream>
#include <set>
#include <unordered_map>
#include <vector>

namespace binrec {
struct MemoryAccess {
    uint64_t pc;
    int64_t offset;
    bool isWrite;
    bool isLocalAccess;
    uint64_t size;
    bool isDirect;
    uint64_t fnBase;
};

struct Successor {
    uint64_t pc;
    uint64_t successor;

    auto operator<(const Successor &other) const -> bool;
};

struct FunctionLog {
    std::vector<uint64_t> entries;
    std::set<std::pair<uint64_t, uint64_t>> entryToCaller;
    std::set<std::pair<uint64_t, uint64_t>> entryToReturn;
    std::set<std::pair<uint64_t, uint64_t>> callerToFollowUp;
    std::map<uint64_t, std::set<uint64_t>> entryToTbs;
};

struct TraceInfo {
    static constexpr const char *defaultFilename = "traceInfo.json";
    static auto get() -> std::shared_ptr<TraceInfo>;

    std::unordered_map<std::string, std::uint32_t> stackFrameSizes;
    std::unordered_map<std::string, std::uint32_t> stackDifference;
    std::vector<MemoryAccess> memoryAccesses;
    std::set<Successor> successors;
    FunctionLog functionLog;

    void add(const TraceInfo &ti);
};

auto operator<<(std::ostream &os, const TraceInfo &ti) -> std::ostream &;
auto operator>>(std::istream &is, TraceInfo &ti) -> std::istream &;
} // namespace binrec

#endif
