#include "link_error.hpp"
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/ManagedStatic.h>

using namespace binrec;
using namespace llvm;
using namespace std;

namespace {
    class LinkErrorCategory : public error_category {
        [[nodiscard]] auto name() const noexcept -> const char * override
        {
            return "binrec.link";
        }

        [[nodiscard]] auto message(int condition) const -> string override
        {
            switch (static_cast<LinkError>(condition)) {
            case LinkError::Bad_Elf:
                return "Bad elf";
            case LinkError::CC_Err:
                return "Error running cc";
            }
            llvm_unreachable("Unknown error type!");
        }
    };
} // namespace

static ManagedStatic<LinkErrorCategory> Category;

auto binrec::make_error_code(LinkError e) noexcept -> error_code
{
    return error_code{static_cast<int>(e), *Category};
}
