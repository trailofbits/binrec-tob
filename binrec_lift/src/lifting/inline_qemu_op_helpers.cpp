#include "inline_qemu_op_helpers.hpp"
#include <array>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace binrec;
using namespace llvm;
using namespace std;

static constexpr array<StringRef, 7> Qemu_Op_Helper_Names = {
    "helper_divb_AL",
    "helper_idivb_AL",
    "helper_divw_AX",
    "helper_idivw_AX",
    "helper_divl_EAX",
    "helper_idivl_EAX",
    "helper_cc_compute"};

// NOLINTNEXTLINE
auto InlineQemuOpHelpersPass::run(Module &m, ModuleAnalysisManager &am) -> PreservedAnalyses
{
    vector<Function *> helper_functions;
    for (Function &f : m) {
        if (any_of(
                Qemu_Op_Helper_Names.begin(),
                Qemu_Op_Helper_Names.end(),
                [&f](StringRef helper_name) { return f.getName().startswith(helper_name); }))
        {
            helper_functions.push_back(&f);
        }
    }

    for (Function *helper : helper_functions) {
        vector<User *> users{helper->user_begin(), helper->user_end()};
        for (User *user : users) {
            auto &call = *cast<CallInst>(user);
            InlineFunctionInfo ifi;
            InlineResult result = InlineFunction(call, ifi);
            assert(result.isSuccess());
        }

        assert(helper->user_empty());
        helper->eraseFromParent();
    }
    return PreservedAnalyses::none();
}
