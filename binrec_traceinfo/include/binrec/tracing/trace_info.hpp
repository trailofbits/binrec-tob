#ifndef BINREC_TRACE_INFO_HPP
#define BINREC_TRACE_INFO_HPP

#include <cstdint>
#include <istream>
#include <map>
#include <memory>
#include <nlohmann/json.hpp>
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
    void to_json(nlohmann::json &j, const MemoryAccess &ma);
    void from_json(const nlohmann::json &j, MemoryAccess &ma);

    struct Successor {
        uint64_t pc;
        uint64_t successor;

        auto operator<(const Successor &other) const -> bool;
    };
    void to_json(nlohmann::json &j, const Successor &s);
    void from_json(const nlohmann::json &j, Successor &s);

    struct FunctionLog {
        std::vector<uint64_t> entries;
        std::set<std::pair<uint64_t, uint64_t>> entryToCaller;
        std::set<std::pair<uint64_t, uint64_t>> entryToReturn;
        std::set<std::pair<uint64_t, uint64_t>> callerToFollowUp;
        std::map<uint64_t, std::set<uint64_t>> entryToTbs;
    };
    void to_json(nlohmann::json &j, const FunctionLog &s);
    void from_json(const nlohmann::json &j, FunctionLog &s);


    struct TraceInfo {
        static constexpr const char *defaultFilename = "traceInfo.json";
        static constexpr const char *defaultName = "traceInfo";
        static constexpr const char *defaultSuffix = ".json";
        static auto get() -> std::shared_ptr<TraceInfo>;

        std::unordered_map<std::string, std::uint32_t> stackFrameSizes;
        std::unordered_map<std::string, std::uint32_t> stackDifference;
        std::vector<MemoryAccess> memoryAccesses;
        std::set<Successor> successors;
        FunctionLog functionLog;

        void restoreFromCopy(TraceInfo *copyTi);
        auto getCopy() -> TraceInfo *;
        void add(const TraceInfo &ti);
    };
    void to_json(nlohmann::json &j, const TraceInfo &s);
    void from_json(const nlohmann::json &j, TraceInfo &s);

    auto operator<<(std::ostream &os, const TraceInfo &ti) -> std::ostream &;
    auto operator>>(std::istream &is, TraceInfo &ti) -> std::istream &;
} // namespace binrec

#endif
