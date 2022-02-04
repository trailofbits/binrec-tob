// #include <gtest/gtest.h>
#include "binrec/tracing/trace_info.hpp"
#include <gmock/gmock.h>
#include <nlohmann/json.hpp>
#include <sstream>

using nlohmann::json;
using ::testing::ElementsAre;
using ::testing::EndsWith;
using ::testing::Pair;
using ::testing::StartsWith;


namespace binrec {
    namespace {

        TEST(trace_info_json, successor_to_json)
        {
            Successor obj{100, 200};
            json actual = obj;
            json expected{{"pc", 100}, {"successor", 200}};
            EXPECT_EQ(actual, expected);
        }

        TEST(trace_info_json, successor_from_json)
        {
            json j = json::parse(R"({"pc": 100, "successor": 200})");
            Successor actual = j.get<Successor>();
            EXPECT_EQ(actual.pc, 100);
            EXPECT_EQ(actual.successor, 200);
        }

        TEST(trace_info_json, memory_access_to_json)
        {
            MemoryAccess ma{100, 200, true, true, 300, true, 400};
            json actual = ma;
            json expected{
                {"pc", 100},
                {"offset", 200},
                {"isWrite", true},
                {"isLocalAccess", true},
                {"size", 300},
                {"isDirect", true},
                {"fnBase", 400}};
            EXPECT_EQ(actual, expected);
        }

        TEST(trace_info_json, memory_access_from_json)
        {
            json j = json::parse(R"({
        "pc": 100,
        "offset": 200,
        "isWrite": true,
        "isLocalAccess": true,
        "size": 300,
        "isDirect": true,
        "fnBase": 400
    })");
            MemoryAccess ma = j.get<MemoryAccess>();
            EXPECT_EQ(ma.pc, 100);
            EXPECT_EQ(ma.offset, 200);
            EXPECT_EQ(ma.isWrite, true);
            EXPECT_EQ(ma.isLocalAccess, true);
            EXPECT_EQ(ma.size, 300);
            EXPECT_EQ(ma.isDirect, true);
            EXPECT_EQ(ma.fnBase, 400);
        }

        TEST(trace_info_json, function_log_to_json)
        {
            FunctionLog obj{{1, 2, 3}, {{4, 5}}, {{6, 7}}, {{8, 9}}, {{10, {11, 12}}}};
            json actual = obj;
            json expected{
                {"entries", {1, 2, 3}},
                {"entryToCaller", {{4, 5}}},
                {"entryToReturn", {{6, 7}}},
                {"callerToFollowUp", {{8, 9}}},
                {"entryToTbs", {{10, {11, 12}}}}};
            EXPECT_EQ(actual, expected);
        }

        TEST(trace_info_json, function_log_from_json)
        {
            json j = json::parse(R"({
        "entries": [1, 2, 3],
        "entryToCaller": [[4, 5]],
        "entryToReturn": [[6, 7]],
        "callerToFollowUp": [[8, 9]],
        "entryToTbs": [[10, [11, 12]]]
    })");
            FunctionLog fl = j.get<FunctionLog>();
            EXPECT_THAT(fl.entries, ElementsAre(1, 2, 3));
            EXPECT_THAT(fl.entryToCaller, ElementsAre(Pair(4, 5)));
            EXPECT_THAT(fl.entryToReturn, ElementsAre(Pair(6, 7)));
            EXPECT_THAT(fl.callerToFollowUp, ElementsAre(Pair(8, 9)));
            EXPECT_THAT(fl.entryToTbs, ElementsAre(Pair(10, ElementsAre(11, 12))));
        }

        TEST(trace_info_json, trace_info_to_json)
        {
            TraceInfo obj{
                {{"hello", 1}},
                {{"asdf", 2}},
                {{100, 200, true, true, 300, true, 400}},
                {{500, 600}},
                {{1, 2, 3}, {{4, 5}}, {{6, 7}}, {{8, 9}}, {{10, {11, 12}}}}};
            json actual = obj;
            json expected{
                {"stackSizes", {{"hello", 1}}},
                {"stackDifference", {{"asdf", 2}}},
                {"memoryAccesses",
                 {{{"pc", 100},
                   {"offset", 200},
                   {"isWrite", true},
                   {"isLocalAccess", true},
                   {"size", 300},
                   {"isDirect", true},
                   {"fnBase", 400}}}},
                {"successors", {{{"pc", 500}, {"successor", 600}}}},
                {"functionLog",
                 {{"entries", {1, 2, 3}},
                  {"entryToCaller", {{4, 5}}},
                  {"entryToReturn", {{6, 7}}},
                  {"callerToFollowUp", {{8, 9}}},
                  {"entryToTbs", {{10, {11, 12}}}}}}};

            EXPECT_EQ(actual, expected) << expected;
        }

        TEST(trace_info_json, trace_info_from_json)
        {
            json j = json::parse(R"({
        "functionLog": {
            "callerToFollowUp":[[8,9]],
            "entries":[1,2,3],
            "entryToCaller":[[4,5]],
            "entryToReturn":[[6,7]],
            "entryToTbs":[[10,[11,12]]]
        },
        "memoryAccesses":[{
            "fnBase":400,
            "isDirect":true,
            "isLocalAccess":true,
            "isWrite":true,
            "offset":200,
            "pc":100,
            "size":300
        }],
        "stackDifference":{"asdf":2},
        "stackSizes":{"hello":1},
        "successors":[{"pc":500,"successor":600}]
    })");
            TraceInfo ti = j.get<TraceInfo>();
            FunctionLog &fl = ti.functionLog;

            EXPECT_THAT(fl.entries, ElementsAre(1, 2, 3));
            EXPECT_THAT(fl.entryToCaller, ElementsAre(Pair(4, 5)));
            EXPECT_THAT(fl.entryToReturn, ElementsAre(Pair(6, 7)));
            EXPECT_THAT(fl.callerToFollowUp, ElementsAre(Pair(8, 9)));
            EXPECT_THAT(fl.entryToTbs, ElementsAre(Pair(10, ElementsAre(11, 12))));

            ASSERT_EQ(ti.memoryAccesses.size(), 1);

            MemoryAccess &ma = ti.memoryAccesses[0];
            EXPECT_EQ(ma.pc, 100);
            EXPECT_EQ(ma.offset, 200);
            EXPECT_EQ(ma.isWrite, true);
            EXPECT_EQ(ma.isLocalAccess, true);
            EXPECT_EQ(ma.size, 300);
            EXPECT_EQ(ma.isDirect, true);
            EXPECT_EQ(ma.fnBase, 400);

            EXPECT_THAT(ti.stackFrameSizes, ElementsAre(Pair("hello", 1)));
            EXPECT_THAT(ti.stackDifference, ElementsAre(Pair("asdf", 2)));

            ASSERT_EQ(ti.successors.size(), 1);

            const Successor &successor = *(ti.successors.begin());
            EXPECT_EQ(successor.pc, 500);
            EXPECT_EQ(successor.successor, 600);
        }

        TEST(trace_info_json, trace_info_istream_load)
        {
            std::stringstream ss{R"({
        "functionLog": {
            "callerToFollowUp":[[8,9]],
            "entries":[1,2,3],
            "entryToCaller":[[4,5]],
            "entryToReturn":[[6,7]],
            "entryToTbs":[[10,[11,12]]]
        },
        "memoryAccesses":[{
            "fnBase":400,
            "isDirect":true,
            "isLocalAccess":true,
            "isWrite":true,
            "offset":200,
            "pc":100,
            "size":300
        }],
        "stackDifference":{"asdf":2},
        "stackSizes":{"hello":1},
        "successors":[{"pc":500,"successor":600}]
    })"};
            TraceInfo ti;

            ss >> ti;

            FunctionLog &fl = ti.functionLog;

            EXPECT_THAT(fl.entries, ElementsAre(1, 2, 3));
            EXPECT_THAT(fl.entryToCaller, ElementsAre(Pair(4, 5)));
            EXPECT_THAT(fl.entryToReturn, ElementsAre(Pair(6, 7)));
            EXPECT_THAT(fl.callerToFollowUp, ElementsAre(Pair(8, 9)));
            EXPECT_THAT(fl.entryToTbs, ElementsAre(Pair(10, ElementsAre(11, 12))));

            ASSERT_EQ(ti.memoryAccesses.size(), 1);

            MemoryAccess &ma = ti.memoryAccesses[0];
            EXPECT_EQ(ma.pc, 100);
            EXPECT_EQ(ma.offset, 200);
            EXPECT_EQ(ma.isWrite, true);
            EXPECT_EQ(ma.isLocalAccess, true);
            EXPECT_EQ(ma.size, 300);
            EXPECT_EQ(ma.isDirect, true);
            EXPECT_EQ(ma.fnBase, 400);

            EXPECT_THAT(ti.stackFrameSizes, ElementsAre(Pair("hello", 1)));
            EXPECT_THAT(ti.stackDifference, ElementsAre(Pair("asdf", 2)));

            ASSERT_EQ(ti.successors.size(), 1);

            const Successor &successor = *(ti.successors.begin());
            EXPECT_EQ(successor.pc, 500);
            EXPECT_EQ(successor.successor, 600);
        }

        TEST(trace_info_json, trace_info_ostream_write)
        {
            std::stringstream ss;

            TraceInfo ti;
            ss << ti;

            std::string content = ss.str();
            EXPECT_THAT(content, StartsWith("{"));
            EXPECT_THAT(content, EndsWith("}"));
        }

    } // namespace
} // namespace binrec
