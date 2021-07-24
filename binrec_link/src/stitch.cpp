#include "stitch.hpp"
#include <llvm/Object/ELFObjectFile.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/Process.h>

using namespace llvm;
using namespace llvm::object;
using namespace std;

namespace {
    struct Patch {
        uint64_t target_offset{};
        uint64_t source_offset{};
        uint64_t length{};
        uint64_t zero_suffix_length = 0;
        bool from_original = false;
    };
} // namespace

template <class ELFT> static auto get_entry_patch() -> Patch
{
    Patch p;
    p.target_offset = offsetof(typename ELFFile<ELFT>::Elf_Ehdr, e_entry);
    p.source_offset = p.target_offset;
    p.length = sizeof(typename ELFFile<ELFT>::Elf_Addr);
    p.from_original = true;
    return p;
}

template <class ELFT>
static auto get_section(const ELFFile<ELFT> &elf, StringRef name) -> const
    typename ELFFile<ELFT>::Elf_Shdr *
{
    const typename ELFFile<ELFT>::Elf_Shdr *result = nullptr;

    auto sections = elf.sections();
    cantFail(sections.takeError());
    for (auto &section : *sections) {
        auto section_name = elf.getSectionName(section);
        if (!section_name) {
            continue;
        }
        if (*section_name == name) {
            result = &section;
            break;
        }
    }
    return result;
}

template <class ELFT>
static auto get_symbol(
    const ELFFile<ELFT> &elf,
    const typename ELFFile<ELFT>::Elf_Shdr &symtab,
    StringRef symbol_name) -> pair<uint64_t, const typename ELFFile<ELFT>::Elf_Sym *>
{
    auto str_tab = elf.getStringTableForSymtab(symtab);
    if (!str_tab) {
        return {0, nullptr};
    }

    auto symbols = elf.symbols(&symtab);
    cantFail(symbols.takeError());
    uint64_t index = 0;
    for (auto &symbol : *symbols) {
        auto name = symbol.getName(*str_tab);
        if (!name) {
            continue;
        }
        if (*name == symbol_name) {
            return {index, &symbol};
        }
        ++index;
    }

    return {0, nullptr};
}

template <class ELFT>
static auto get_start_patch(const ELFFile<ELFT> &original_elf, const ELFFile<ELFT> &recovered_elf)
    -> Patch
{
    auto *recovered_symtab_section = get_section(recovered_elf, ".symtab");
    assert(recovered_symtab_section && "Recovered binary must have a symtab section.");

    auto recovered_main_symbol = get_symbol(recovered_elf, *recovered_symtab_section, "main");
    assert(recovered_main_symbol.second && "Recovered binary must have a main symbol.");

    uint64_t recovered_main_address_offset = recovered_symtab_section->sh_offset +
        recovered_main_symbol.first * recovered_symtab_section->sh_entsize +
        offsetof(typename ELFFile<ELFT>::Elf_Sym, st_value);

    auto *original_text_section = get_section(recovered_elf, ".orig.text");
    assert(original_text_section && "Recovered binary must have a .orig.text section.");

    auto original_start_address = original_elf.getHeader().e_entry;

    Patch p;
    // 44 is the offset for the address of `PUSH main` from the beginning of _start in binaries
    // generated with GCC 9.3.0 on Ubuntu 20.04. Debian 10 is 40. Adjust for your usecase.
    p.target_offset = original_start_address - original_text_section->sh_addr +
        original_text_section->sh_offset + 44;
    p.source_offset = recovered_main_address_offset;
    p.length = sizeof(typename ELFFile<ELFT>::Elf_Addr);
    return p;
}

template <class ELFT> static auto get_dynamic_patch(const ELFFile<ELFT> &recovered_elf) -> Patch
{
    auto *original_dynamic_section = get_section(recovered_elf, ".orig.dynamic");
    if (original_dynamic_section == nullptr) {
        return {};
    }

    auto *recovered_dynamic_section = get_section(recovered_elf, ".dynamic");
    assert(recovered_dynamic_section && "Recovered binary must have a dynamic section.");

    Patch p;
    p.target_offset = recovered_dynamic_section->sh_offset;
    p.source_offset = original_dynamic_section->sh_offset;
    p.length = min(original_dynamic_section->sh_size, recovered_dynamic_section->sh_size);
    if (original_dynamic_section->sh_size <= recovered_dynamic_section->sh_size) {
        p.zero_suffix_length =
            recovered_dynamic_section->sh_size - original_dynamic_section->sh_size;
    } else {
        // FPar: Usually the .dynamic section ends in a bunch of zeros, so when copying it, that
        // should not cause problems. However, I leave this message here, just in case this
        // causes a problem.
        errs() << "Warning: size of .dynamic is smaller than .orig.dynamic.\n";
    }
    return p;
}

template <class ELFT>
static auto get_patches(const ELFFile<ELFT> &original_elf, const Binary *recovered_binary)
    -> SmallVector<Patch, 3>
{
    SmallVector<Patch, 3> patches;
    auto &recovered_elf = cast<const ELFObjectFile<ELFT>>(recovered_binary)->getELFFile();
    // patches.push_back(get_entry_patch<ELFT>());
    // patches.push_back(get_start_patch(original_elf, recovered_elf));
    //  patches.push_back(get_dynamic_patch(recovered_elf));

    return patches;
}

static auto get_patches(const Binary *original_binary, const Binary *recovered_binary)
    -> SmallVector<Patch, 3>
{
    if (auto *elf32le = dyn_cast<ELF32LEObjectFile>(original_binary))
        return get_patches(elf32le->getELFFile(), recovered_binary);
    if (auto *elf32be = dyn_cast<ELF32BEObjectFile>(original_binary))
        return get_patches(elf32be->getELFFile(), recovered_binary);
    if (auto *elf64le = dyn_cast<ELF64LEObjectFile>(original_binary))
        return get_patches(elf64le->getELFFile(), recovered_binary);
    if (auto *elf64be = dyn_cast<ELF64BEObjectFile>(original_binary))
        return get_patches(elf64be->getELFFile(), recovered_binary);
    llvm_unreachable("Binary is not an ELF file.");
}

auto binrec::stitch(StringRef binary_path, LinkContext &ctx) -> error_code
{
    auto patches = get_patches(ctx.original_binary.getBinary(), ctx.recovered_binary.getBinary());

    uint64_t binary_size = 0;
    error_code ec = sys::fs::file_size(binary_path, binary_size);
    if (ec) {
        return ec;
    }

    int fd = 0;
    ec = sys::fs::openFileForReadWrite(binary_path, fd, sys::fs::CD_OpenExisting, sys::fs::OF_None);
    if (ec) {
        return ec;
    }

    sys::fs::mapped_file_region
        mapped_binary{fd, sys::fs::mapped_file_region::readwrite, binary_size, 0, ec};
    error_code ec2 = sys::Process::SafelyCloseFileDescriptor(fd);
    if (ec) {
        return ec;
    }
    if (ec2) {
        return ec2;
    }

    char *data = mapped_binary.data();
    for (auto &patch : patches) {
        const char *source_begin;
        if (patch.from_original) {
            source_begin = ctx.original_binary.getBinary()->getData().data() + patch.source_offset;
        } else {
            source_begin = data + patch.source_offset;
        }
        copy(source_begin, source_begin + patch.length, data + patch.target_offset);
    }

    return error_code();
}
