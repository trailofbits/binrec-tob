#ifndef BINREC_GLOBAL_ADDRESS_MAP_HPP
#define BINREC_GLOBAL_ADDRESS_MAP_HPP

#include <llvm/ADT/StringRef.h>
#include <llvm/Object/ELFObjectFile.h>
#include <map>

namespace binrec {
    class GlobalAddressMap {
    public:
        struct Offset {
            llvm::StringRef symbol;
            uint64_t offset;
        };

        explicit GlobalAddressMap(const llvm::object::ELFObjectFileBase &elf);
        [[nodiscard]] auto get(uint64_t absoluteAddress) const -> std::optional<Offset>;

    private:
        template <typename ELFT> void getSections(const llvm::object::ELFFile<ELFT> &elf);

        struct Section {
            llvm::SmallString<128> symbol;
            uint64_t size;
        };

        std::map<uint64_t, Section> sectionMap;
    };
} // namespace binrec

#endif
