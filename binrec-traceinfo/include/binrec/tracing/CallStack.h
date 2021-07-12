#ifndef BINREC_CALLSTACK_H
#define BINREC_CALLSTACK_H

#include "StackFrame.h"
#include "binrec/Address.h"
#include "binrec/ByteUnit.h"
#include <functional>
#include <map>
#include <memory>
#include <vector>

namespace binrec {
class CallStack {
    struct LiveStackFrame {
        StackFrame *const frame;
        std::unique_ptr<LiveStackFrame> parent;
        const StackAddress base;
        StackAddress top;
        InstructionAddress inCallAt;

        LiveStackFrame(StackFrame *frame, std::unique_ptr<LiveStackFrame> &&parent, StackAddress base);

        [[nodiscard]] auto range() const -> StackFrameRange;
        [[nodiscard]] auto inCall() const -> bool;
    };

    StackAddress stackBase;
    std::unique_ptr<LiveStackFrame> topFrame;
    std::map<StackAddress, LiveStackFrame *, std::greater<>> activeFrames;
    std::map<StackFrame::Id, std::unique_ptr<StackFrame>> frames;

public:
    CallStack();

private:
    [[nodiscard]] auto find(StackAddress accessAddress) const -> LiveStackFrame *;

public:
    [[nodiscard]] auto pointsToStack(StackAddress pointer) const -> bool;

    void recordCall(FunctionAddress target, InstructionAddress callSite, StackAddress top);
    void recordReturn(FunctionAddress from);
    void recordStackPointerUpdate(InstructionAddress at, StackAddress value);
    void recordFramePointerUpdate(InstructionAddress at, StackAddress value);
    void recordMemoryWrite(InstructionAddress at, StackAddress location);

    [[nodiscard]] auto data() const -> std::vector<StackFrame::Data>;
};
} // namespace binrec

#endif
