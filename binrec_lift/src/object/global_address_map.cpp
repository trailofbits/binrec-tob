#include "global_address_map.hpp"

using namespace binrec;
using namespace llvm;
using namespace std;

GlobalAddressMap::GlobalAddressMap(const object::ELFObjectFileBase &elf)
{
    if (const auto *elf32le = dyn_cast<object::ELF32LEObjectFile>(&elf))
        getSections(elf32le->getELFFile());
    else if (const auto *elf32be = dyn_cast<object::ELF32BEObjectFile>(&elf))
        getSections(elf32be->getELFFile());
    else if (const auto *elf64le = dyn_cast<object::ELF64LEObjectFile>(&elf))
        getSections(elf64le->getELFFile());
    else if (const auto *elf64be = dyn_cast<object::ELF64BEObjectFile>(&elf))
        getSections(elf64be->getELFFile());
    else
        llvm_unreachable("Bad ELF");
}

auto GlobalAddressMap::get(uint64_t absoluteAddress) const -> optional<Offset>
{
    auto it = sectionMap.lower_bound(absoluteAddress);
    if (it->first == absoluteAddress) {
        return Offset{it->second.symbol, 0};
    }
    if (it == sectionMap.begin()) {
        return {};
    }
    --it;
    if (absoluteAddress >= it->first + it->second.size) {
        return {};
    }
    return Offset{it->second.symbol, absoluteAddress - it->first};
}

template <typename ELFT> void GlobalAddressMap::getSections(const object::ELFFile<ELFT> &elf)
{
    Expected<typename ELFT::ShdrRange> sections = elf.sections();
    cantFail(sections.takeError());
    for (const typename ELFT::Shdr &elfSec : *sections) {
        if (elfSec.sh_addr == 0)
            continue;

        Expected<StringRef> secName = elf.getSectionName(elfSec);
        if (!secName)
            continue;

        SmallString<128> sectionSymbol{"binrec_section"};
        sectionSymbol.append(*secName);
        replace(sectionSymbol.begin(), sectionSymbol.end(), '.', '_');
        replace(sectionSymbol.begin(), sectionSymbol.end(), '-', '_');

        Section s{move(sectionSymbol), elfSec.sh_size};
        sectionMap.template emplace(elfSec.sh_addr, move(s));
    }
}
