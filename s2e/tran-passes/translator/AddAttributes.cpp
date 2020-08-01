#include "AddAttributes.h"
#include "PassUtils.h"
#include <vector>
#include <string>

using namespace llvm;

char AddAttributes::ID = 0;
static RegisterPass<AddAttributes> X("add-attributes",
        "add function attributes for applications", false, false);

bool AddAttributes::runOnFunction(Function &f) {
    std::vector<std::string> blah;
    blah.push_back("safestack");
    blah.push_back("sanitize-address");
    blah.push_back("fla");
    blah.push_back("sub");
    blah.push_back("bcf");

    if (f.hasName() && f.getName() != "main" &&
            !f.getName().startswith("mywrite") &&
            !f.getName().startswith("__stub")) {
        for(auto const& atr: blah){
            f.addFnAttr(atr);
        }
        return true;
    }

    return false;
}
