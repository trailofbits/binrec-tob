#include "binrec/tracing/CallStack.h"
#include "binrec/tracing/StackFrame.h"
#include <algorithm>
#include <cassert>
#include <iostream>

using namespace binrec;

CallStack::LiveStackFrame::LiveStackFrame(StackFrame *frame, std::unique_ptr<LiveStackFrame> &&parent,
                                          StackAddress base)
    : frame{frame}, parent{move(parent)}, base{base}, top{base}, inCallAt{} {}

auto CallStack::LiveStackFrame::inCall() const -> bool { return inCallAt != InstructionAddress{}; }

auto CallStack::LiveStackFrame::range() const -> StackFrameRange { return {base, top}; }

CallStack::CallStack() : topFrame{new LiveStackFrame{nullptr, nullptr, {}}} {}

auto CallStack::pointsToStack(StackAddress pointer) const -> bool {
    LiveStackFrame *frame = find(pointer);
    return frame != nullptr;
}

void CallStack::recordCall(FunctionAddress target, InstructionAddress callSite, StackAddress top) {
    std::cout << "Record call at " << callSite << " to " << target << " (stack = " << top << ")" << std::endl;
    if (activeFrames.lower_bound(top) != activeFrames.end()) {
        std::cout << "Active: " << activeFrames.rbegin()->second->base.value << ", top: " << top.value << std::endl;
        assert(activeFrames.lower_bound(top) == activeFrames.end() &&
               "There are no active frames with a base beyond the provided stack top.");
    }

    StackFrame::Id frameId{target, callSite};
    auto frame = frames.find(frameId);
    if (frame == frames.end()) {
        frame = frames.emplace(frameId, new StackFrame{target, callSite}).first;
    }
    topFrame->inCallAt = callSite;
    auto liveFrame = std::make_unique<LiveStackFrame>(frame->second.get(), std::move(topFrame), top);
    activeFrames.emplace(liveFrame->base, liveFrame.get());
    topFrame = std::move(liveFrame);
}

void CallStack::recordReturn(FunctionAddress from) {
    std::cout << "Record return from " << from << std::endl;
    assert(topFrame->frame->functionAddress == from);
    activeFrames.erase(topFrame->base);
    std::unique_ptr<LiveStackFrame> tmp = std::move(topFrame);
    topFrame = std::move(tmp->parent);
    topFrame->inCallAt = {};
}

void CallStack::recordStackPointerUpdate(InstructionAddress at, StackAddress value) {
    if (topFrame->frame == nullptr) {
        if (stackBase.value == 0) {
            stackBase = value;
        }
        return;
    }

    std::cout << "Update stack pointer at " << at << " to " << value << " (" << value - topFrame->top << ")"
              << "\n";
    topFrame->frame->recordStackPointerUpdate(at, value - topFrame->base);
    topFrame->top = value;
    if (topFrame->top > topFrame->base) {
        assert(topFrame->parent != nullptr &&
               "Function that changes stack pointer into frame of caller must has a caller frame.");
        if (value == topFrame->parent->base) {
            std::unique_ptr<LiveStackFrame> tmp = std::move(topFrame->parent);
            topFrame->parent = std::move(tmp->parent);
        }
    }
}

void CallStack::recordFramePointerUpdate(InstructionAddress at, StackAddress value) {
    if (topFrame->frame == nullptr) {
        return;
    }
    topFrame->frame->recordFramePointerUpdate(at, value - topFrame->base);
}

void CallStack::recordMemoryWrite(InstructionAddress at, StackAddress location) {
    LiveStackFrame *liveFrame = find(location);
    if (liveFrame == nullptr) {
        return;
    }
}

auto CallStack::data() const -> std::vector<StackFrame::Data> {
    std::vector<StackFrame::Data> result;
    result.reserve(frames.size());
    std::transform(frames.cbegin(), frames.cend(), std::back_inserter(result),
                   [](const auto &entry) -> StackFrame::Data { return entry.second->asData(); });
    return result;
}

auto CallStack::find(StackAddress accessAddress) const -> LiveStackFrame * {
    auto result = activeFrames.lower_bound(accessAddress);
    if (result == activeFrames.end()) {
        return nullptr;
    }
    LiveStackFrame *frame = result->second;
    if (!frame->range().contains(accessAddress)) {
        return nullptr;
    }
    return frame;
}
