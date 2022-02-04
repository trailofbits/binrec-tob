
#include "binrec/tracing/trace_info.hpp"
#include <gmock/gmock.h>


using ::testing::ElementsAre;
using ::testing::Eq;
using ::testing::Field;
using ::testing::Pair;
using ::testing::UnorderedElementsAre;

namespace binrec {
    namespace {

        TEST(trace_info_merge, stack_frame_sizes)
        {
            TraceInfo base;
            base.stackFrameSizes.emplace("first", 1);
            base.stackFrameSizes.emplace("second", 2);

            TraceInfo patch;
            patch.stackFrameSizes.emplace("first", 3);
            patch.stackFrameSizes.emplace("second", 0);
            patch.stackFrameSizes.emplace("third", 1);

            base.add(patch);

            EXPECT_THAT(
                base.stackFrameSizes,
                UnorderedElementsAre(Pair("first", 3), Pair("second", 2), Pair("third", 1)));
        }

        TEST(trace_info_merge, stack_differences)
        {
            TraceInfo base;
            base.stackDifference.emplace("first", 100);
            base.stackDifference.emplace("second", 200);

            TraceInfo patch;
            patch.stackDifference.emplace("first", 100);
            patch.stackDifference.emplace("third", 300);

            base.add(patch);

            EXPECT_THAT(
                base.stackDifference,
                UnorderedElementsAre(Pair("first", 100), Pair("second", 200), Pair("third", 300)));
        }

        TEST(trace_info_merge, stack_differences_failure)
        {
            TraceInfo base;
            base.stackDifference.emplace("first", 100);

            TraceInfo patch;
            patch.stackDifference.emplace("first", 200);
            EXPECT_DEATH(base.add(patch), ".*");
        }

        TEST(trace_info_merge, memory_accesses)
        {
            TraceInfo base;
            MemoryAccess first;
            first.pc = 100;
            base.memoryAccesses.push_back(first);

            TraceInfo patch;
            MemoryAccess second;
            second.pc = 200;
            patch.memoryAccesses.push_back(second);

            base.add(patch);

            ASSERT_EQ(base.memoryAccesses.size(), 2);
            EXPECT_EQ(base.memoryAccesses[0].pc, 100);
            EXPECT_EQ(base.memoryAccesses[1].pc, 200);
        }

        TEST(trace_info_merge, successors)
        {
            TraceInfo base;
            Successor first;
            first.pc = 100;
            base.successors.insert(std::move(first));

            TraceInfo patch;
            Successor second;
            second.pc = 200;
            patch.successors.insert(std::move(second));

            base.add(patch);

            EXPECT_THAT(
                base.successors,
                UnorderedElementsAre(
                    Field(&Successor::pc, Eq(100)),
                    Field(&Successor::pc, Eq(200))));
        }

        TEST(trace_info_merge, function_log_empty_entries)
        {
            TraceInfo base;

            TraceInfo patch;
            patch.functionLog.entries = {1, 2, 3};

            base.add(patch);

            EXPECT_THAT(base.functionLog.entries, ElementsAre(1, 2, 3));
        }

        TEST(trace_info_merge, function_log_entry_to_caller)
        {
            TraceInfo base;
            base.functionLog.entryToCaller = {{1, 2}, {3, 4}};

            TraceInfo patch;
            patch.functionLog.entryToCaller = {{5, 6}, {7, 8}};

            base.add(patch);

            EXPECT_THAT(
                base.functionLog.entryToCaller,
                UnorderedElementsAre(Pair(1, 2), Pair(3, 4), Pair(5, 6), Pair(7, 8)));
        }

        TEST(trace_info_merge, function_log_entry_to_return)
        {
            TraceInfo base;
            base.functionLog.entryToReturn = {{1, 2}, {3, 4}};

            TraceInfo patch;
            patch.functionLog.entryToReturn = {{5, 6}, {7, 8}};

            base.add(patch);

            EXPECT_THAT(
                base.functionLog.entryToReturn,
                UnorderedElementsAre(Pair(1, 2), Pair(3, 4), Pair(5, 6), Pair(7, 8)));
        }

        TEST(trace_info_merge, function_log_caller_to_followup)
        {
            TraceInfo base;
            base.functionLog.callerToFollowUp = {{1, 2}, {3, 4}};

            TraceInfo patch;
            patch.functionLog.callerToFollowUp = {{5, 6}, {7, 8}};

            base.add(patch);

            EXPECT_THAT(
                base.functionLog.callerToFollowUp,
                UnorderedElementsAre(Pair(1, 2), Pair(3, 4), Pair(5, 6), Pair(7, 8)));
        }

        TEST(trace_info_merge, function_log_entry_to_tbs)
        {
            TraceInfo base;
            base.functionLog.entryToTbs = {{1, {2, 3}}, {4, {5, 6}}};

            TraceInfo patch;
            patch.functionLog.entryToTbs = {{1, {3, 7}}, {8, {9, 10}}};

            base.add(patch);

            EXPECT_THAT(
                base.functionLog.entryToTbs,
                UnorderedElementsAre(
                    Pair(1, UnorderedElementsAre(2, 3, 7)),
                    Pair(4, UnorderedElementsAre(5, 6)),
                    Pair(8, UnorderedElementsAre(9, 10))));
        }

        TEST(trace_info_merge, successor_comparison_pc_eq)
        {
            Successor first{1, 2};
            Successor second{1, 3};

            EXPECT_LT(first, second);
        }

        TEST(trace_info_merge, successor_comparison_pc_ne)
        {
            Successor first{2, 1};
            Successor second{1, 2};

            EXPECT_LT(second, first);
        }

    } // namespace
} // namespace binrec
