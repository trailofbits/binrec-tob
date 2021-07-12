#include "LinkError.h"
#include <llvm/Support/ErrorHandling.h>
#include <llvm/Support/ManagedStatic.h>

using namespace binrec;
using namespace llvm;

namespace {
class LinkErrorCategory : public std::error_category {
    [[nodiscard]] auto name() const noexcept -> const char * override { return "binrec.link"; }

    [[nodiscard]] auto message(int condition) const -> std::string override {
        switch (static_cast<LinkError>(condition)) {
        case LinkError::BadElf:
            return "Bad elf";
        case LinkError::CCErr:
            return "Error running cc";
        }
        llvm_unreachable("Unknown error type!");
    }
};

ManagedStatic<LinkErrorCategory> category;
} // namespace

auto binrec::make_error_code(LinkError e) noexcept -> std::error_code {
    return std::error_code{static_cast<int>(e), *category};
}
