#include "InlineWrapper.h"
#include "PassUtils.h"

using namespace llvm;

char InlineWrapper::ID = 0;
static RegisterPass<InlineWrapper> x("inline-wrapper",
        "S2E Tag @wrapper as always-inline (use in combination with -always-inline)",
        false, false);

bool InlineWrapper::runOnModule(Module &m) {

    std::vector<StringRef> funcNames {"main", "myexit", "helper_stub_trampoline", "helper_debug_state"};
    for(Function &F : m) {
        if(F.getName().startswith("llvm") || F.getName().startswith("mywrite") || F.getName().startswith("__stub_") ||
                //F.getName().startswith("helper_cc_compute_c") || /*F.getName().startswith("helper_cc_compute_") ||*/
                std::find(funcNames.begin(), funcNames.end(), F.getName()) != funcNames.end()) 
                continue;
        else { 
            llvm::errs() << "inline: " << F.getName() << "\n";
            F.addFnAttr(Attribute::AlwaysInline);
        }
     }

/*        
     std::vector<StringRef> funcNames {"wrapper"};
     for(Function &F : m) {
           if(F.getName().startswith("helper_cc_compute_") ||
               //F.getName().startswith("uadd") ||
                F.getName().startswith("helper_divl_EAX") ||
                F.getName().startswith("helper_idivl_EAX") ||
                //F.getName().startswith("helper_f") || 
                F.getName().startswith("float32") || 
                F.getName().startswith("floatx80") ||
                F.getName().startswith("roundAndPackFloat") ||
                F.getName().startswith("mywrite") ||
                F.getName().startswith("raise_exception") ||
                std::find(funcNames.begin(), funcNames.end(), F.getName()) != funcNames.end()) {
            llvm::errs() << "inline: " << F.getName() << "\n";
            F.addFnAttr(Attribute::AlwaysInline);
//            if(F.getFnAttribute(Attribute::NoInline) != NULL)
//                F.removeFnAttr(Attribute::NoInline);
        }

    }
*/
    return true;
}

