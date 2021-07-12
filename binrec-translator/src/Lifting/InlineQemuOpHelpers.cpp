#include "RegisterPasses.h"
#include <array>
#include <llvm/ADT/StringRef.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Transforms/Utils/Cloning.h>

using namespace llvm;

using std::vector;

namespace {
constexpr std::array<StringRef, 7> qemuOpHelperNames = {"helper_divb_AL",   "helper_idivb_AL", "helper_divw_AX",
                                                        "helper_idivw_AX",  "helper_divl_EAX", "helper_idivl_EAX",
                                                        "helper_cc_compute"};

class InlineQemuOpHelpersPass : public PassInfoMixin<InlineQemuOpHelpersPass> {
public:
    auto run(Module &m, ModuleAnalysisManager &) -> PreservedAnalyses {
        vector<Function *> helperFunctions;
        for (Function &f : m) {
            if (std::any_of(qemuOpHelperNames.begin(), qemuOpHelperNames.end(),
                            [&f](StringRef helperName) { return f.getName().startswith(helperName); })) {
                helperFunctions.push_back(&f);
            }
        }

        for (Function *helper : helperFunctions) {
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
};
} // namespace

void binrec::addInlineQemuOpHelpersPass(ModulePassManager &mpm) { return mpm.addPass(InlineQemuOpHelpersPass()); }
