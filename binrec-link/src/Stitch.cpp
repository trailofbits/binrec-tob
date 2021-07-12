#include "Stitch.h"
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>

using namespace llvm;

namespace {
struct Patch {
    uint64_t targetOffset{};
    uint64_t sourceOffset{};
    uint64_t length{};
    uint64_t zeroSuffixLength = 0;
    bool fromOriginal = false;
};

template <class ELFT> auto getEntryPatch() -> Patch {
    Patch p;
    p.targetOffset = offsetof(typename object::ELFFile<ELFT>::Elf_Ehdr, e_entry);
    p.sourceOffset = p.targetOffset;
    p.length = sizeof(typename object::ELFFile<ELFT>::Elf_Addr);
    p.fromOriginal = true;
    return p;
}

template <class ELFT>
auto getSection(const object::ELFFile<ELFT> &elf, StringRef name) -> const typename object::ELFFile<ELFT>::Elf_Shdr * {
    const typename object::ELFFile<ELFT>::Elf_Shdr *result = nullptr;

    auto sections = elf.sections();
    cantFail(sections.takeError());
    for (auto &section : *sections) {
        auto sectionName = elf.getSectionName(section);
        if (!sectionName) {
            continue;
        }
        if (*sectionName == name) {
            result = &section;
            break;
        }
    }
    return result;
}

template <class ELFT>
auto getSymbol(const object::ELFFile<ELFT> &elf, const typename object::ELFFile<ELFT>::Elf_Shdr &symtab,
               StringRef symbolName) -> std::pair<uint64_t, const typename object::ELFFile<ELFT>::Elf_Sym *> {
    auto strTab = elf.getStringTableForSymtab(symtab);
    if (!strTab) {
        return {0, nullptr};
    }

    auto symbols = elf.symbols(&symtab);
    cantFail(symbols.takeError());
    uint64_t index = 0;
    for (auto &symbol : *symbols) {
        auto name = symbol.getName(*strTab);
        if (!name) {
            continue;
        }
        if (*name == symbolName) {
            return {index, &symbol};
        }
        ++index;
    }

    return {0, nullptr};
}

template <class ELFT>
auto getStartPatch(const object::ELFFile<ELFT> &originalELF, const object::ELFFile<ELFT> &recoveredELF) -> Patch {
    auto *recoveredSymtabSection = getSection(recoveredELF, ".symtab");
    assert(recoveredSymtabSection && "Recovered binary must have a symtab section.");

    auto recoveredMainSymbol = getSymbol(recoveredELF, *recoveredSymtabSection, "main");
    assert(recoveredMainSymbol.second && "Recovered binary must have a main symbol.");

    uint64_t recoveredMainAddressOffset = recoveredSymtabSection->sh_offset +
                                          recoveredMainSymbol.first * recoveredSymtabSection->sh_entsize +
                                          offsetof(typename object::ELFFile<ELFT>::Elf_Sym, st_value);

    auto *originalTextSection = getSection(recoveredELF, ".orig.text");
    assert(originalTextSection && "Recovered binary must have a .orig.text section.");

    auto originalStartAddress = originalELF.getHeader().e_entry;

    Patch p;
    // 44 is the offset for the address of `PUSH main` from the beginning of _start in binaries generated with GCC 9.3.0
    // on Ubuntu 20.04. Debian 10 is 40. Adjust for your usecase.
    p.targetOffset = originalStartAddress - originalTextSection->sh_addr + originalTextSection->sh_offset + 40;
    p.sourceOffset = recoveredMainAddressOffset;
    p.length = sizeof(typename object::ELFFile<ELFT>::Elf_Addr);
    return p;
}

template <class ELFT> auto getDynamicPatch(const object::ELFFile<ELFT> &recoveredELF) -> Patch {
    auto *originalDynamicSection = getSection(recoveredELF, ".orig.dynamic");
    if (originalDynamicSection == nullptr) {
        return {};
    }

    auto *recoveredDynamicSection = getSection(recoveredELF, ".dynamic");
    assert(recoveredDynamicSection && "Recovered binary must have a dynamic section.");

    Patch p;
    p.targetOffset = recoveredDynamicSection->sh_offset;
    p.sourceOffset = originalDynamicSection->sh_offset;
    p.length = std::min(originalDynamicSection->sh_size, recoveredDynamicSection->sh_size);
    if (originalDynamicSection->sh_size <= recoveredDynamicSection->sh_size) {
        p.zeroSuffixLength = recoveredDynamicSection->sh_size - originalDynamicSection->sh_size;
    } else {
        // FPar: Usually the .dynamic section ends in a bunch of zeros, so when copying it, that should not cause
        // problems. However, I leave this message here, just in case this causes a problem.
        errs() << "Warning: size of .dynamic is smaller than .orig.dynamic.\n";
    }
    return p;
}

template <class ELFT>
auto getPatches(const object::ELFFile<ELFT> &originalELF, const object::Binary *recoveredBinary)
    -> SmallVector<Patch, 3> {
    SmallVector<Patch, 3> patches;
    auto &recoveredELF = cast<const object::ELFObjectFile<ELFT>>(recoveredBinary)->getELFFile();
    patches.push_back(getEntryPatch<ELFT>());
    patches.push_back(getStartPatch(originalELF, recoveredELF));
    patches.push_back(getDynamicPatch(recoveredELF));

    return patches;
}

auto getPatches(const object::Binary *originalBinary, const object::Binary *recoveredBinary) -> SmallVector<Patch, 3> {
    if (auto *elf32le = dyn_cast<object::ELF32LEObjectFile>(originalBinary))
        return getPatches(elf32le->getELFFile(), recoveredBinary);
    if (auto *elf32be = dyn_cast<object::ELF32BEObjectFile>(originalBinary))
        return getPatches(elf32be->getELFFile(), recoveredBinary);
    if (auto *elf64le = dyn_cast<object::ELF64LEObjectFile>(originalBinary))
        return getPatches(elf64le->getELFFile(), recoveredBinary);
    if (auto *elf64be = dyn_cast<object::ELF64BEObjectFile>(originalBinary))
        return getPatches(elf64be->getELFFile(), recoveredBinary);
    llvm_unreachable("Binary is not an ELF file.");
}
} // namespace

auto binrec::stitch(llvm::StringRef binaryPath, LinkContext &ctx) -> std::error_code {
    auto patches = getPatches(ctx.originalBinary.getBinary(), ctx.recoveredBinary.getBinary());

    uint64_t binarySize = 0;
    std::error_code ec = sys::fs::file_size(binaryPath, binarySize);
    if (ec) {
        return ec;
    }

    int fd = 0;
    ec = sys::fs::openFileForReadWrite(binaryPath, fd, sys::fs::CD_OpenExisting, sys::fs::OF_None);
    if (ec) {
        return ec;
    }

    sys::fs::mapped_file_region mappedBinary{fd, sys::fs::mapped_file_region::readwrite, binarySize, 0, ec};
    std::error_code ec2 = sys::Process::SafelyCloseFileDescriptor(fd);
    if (ec) {
        return ec;
    }
    if (ec2) {
        return ec2;
    }

    char *data = mappedBinary.data();
    for (auto &patch : patches) {
        const char *sourceBegin = nullptr;
        if (patch.fromOriginal) {
            sourceBegin = ctx.originalBinary.getBinary()->getData().data() + patch.sourceOffset;
        } else {
            sourceBegin = data + patch.sourceOffset;
        }
        std::copy(sourceBegin, sourceBegin + patch.length, data + patch.targetOffset);
    }

    return std::error_code();
}
