#include "elf_exe_to_obj.hpp"
#include "compiler_command.hpp"
#include "link_error.hpp"
#include <llvm/Object/ELFObjectFile.h>

using namespace binrec;
using namespace llvm;
using namespace llvm::object;
using namespace std;

namespace {
    struct SectionRange {
        uint64_t offset = 0;
        uint64_t length = 0;
        SectionInfo section;
    };

    struct SectionFile {
        SectionInfo section;
        SmallString<128> path;
    };
} // namespace

template <class ELFT> static auto get_ranges(const ELFFile<ELFT> &elf_exe) -> vector<SectionRange>
{
    vector<SectionRange> ranges;

    auto sections = elf_exe.sections();
    cantFail(sections.takeError());
    for (const auto &elf_sec : *sections) {
        if (elf_sec.sh_addr == 0)
            continue;

        Expected<StringRef> sec_name = elf_exe.getSectionName(elf_sec);
        if (!sec_name)
            continue;

        SectionRange range;
        range.offset = elf_sec.sh_offset;
        range.length = elf_sec.sh_size;

        range.section.name = *sec_name;
        range.section.virtual_address = elf_sec.sh_addr;
        range.section.flags = elf_sec.sh_flags;
        range.section.size = elf_sec.sh_size;
        range.section.nobits = elf_sec.sh_type == ELF::SHT_NOBITS;

        ranges.push_back(range);
    }

    return ranges;
}

static auto
copy_ranges(const char *exe_data, const vector<SectionRange> &ranges, StringRef work_dir)
    -> ErrorOr<vector<SectionFile>>
{
    vector<SectionFile> section_files;

    for (const auto &range : ranges) {
        SectionFile sf;
        sf.section = range.section;

        if (!range.section.nobits) {
            sf.path = work_dir;
            sf.path += "/section-";
            sf.path += range.section.name;
            sf.path += ".bin";

            error_code ec;
            raw_fd_ostream os(sf.path, ec);
            if (ec) {
                return ec;
            }
            os.write(exe_data + range.offset, range.length);
        }

        section_files.push_back(sf);
    }

    return section_files;
}

static auto copy_sections(LinkContext &ctx) -> ErrorOr<vector<SectionFile>>
{
    Binary *exe = ctx.original_binary.getBinary();
    if (!exe->isELF()) {
        return LinkError::Bad_Elf;
    }

    vector<SectionRange> ranges;
    if (const ELF32LEObjectFile *elf32le = dyn_cast<ELF32LEObjectFile>(exe)) {
        ranges = get_ranges(elf32le->getELFFile());
    } else if (const ELF32BEObjectFile *elf32be = dyn_cast<ELF32BEObjectFile>(exe)) {
        ranges = get_ranges(elf32be->getELFFile());
    } else if (const ELF64LEObjectFile *elf64le = dyn_cast<ELF64LEObjectFile>(exe)) {
        ranges = get_ranges(elf64le->getELFFile());
    } else if (const ELF64BEObjectFile *elf64be = dyn_cast<ELF64BEObjectFile>(exe)) {
        ranges = get_ranges(elf64be->getELFFile());
    } else {
        return LinkError::Bad_Elf;
    }

    const char *exe_data = exe->getData().data();
    return copy_ranges(exe_data, ranges, ctx.work_dir);
}

static auto section_flags_to_str(uint64_t flags) -> string
{
    string result;
    if (!(flags & ELF::SHF_MASKPROC)) {
        if (flags & ELF::SHF_WRITE) {
            result += "w";
        }
        if (flags & ELF::SHF_ALLOC) {
            result += "a";
        }
        if (flags & ELF::SHF_EXECINSTR) {
            result += "x";
        }
    }
    return result;
}

static auto
make_object(const vector<SectionFile> &section_files, StringRef object_filename, StringRef work_dir)
    -> error_code
{
    SmallString<128> assembly_filename = work_dir;
    assembly_filename.append("/originalSections.S");

    error_code ec;
    raw_fd_ostream assembly_file(assembly_filename, ec);
    if (ec) {
        return ec;
    }

    for (const SectionFile &file : section_files) {
        const SectionInfo &section = file.section;

        const char *progbits = section.nobits ? "" : ",@progbits";

        auto normalized_section_name = section.name;
        replace(normalized_section_name.begin(), normalized_section_name.end(), '.', '_');
        replace(normalized_section_name.begin(), normalized_section_name.end(), '-', '_');

        assembly_file << "\t.global binrec_section" << normalized_section_name << "\n"
                      << "\t.section\t.orig" << section.name << ",\""
                      << section_flags_to_str(section.flags) << "\"" << progbits << "\n"
                      << "binrec_section" << normalized_section_name << ":\n";

        if (section.nobits) {
            assembly_file << "\t.zero " << section.size << '\n';
        } else {
            assembly_file << "\t.incbin \"" << file.path << "\"\n";
        }

        assembly_file << "\n";
    }
    assembly_file.close();

    CompilerCommand cc{CompilerCommandMode::Compile};
    cc.output_path = object_filename;
    cc.input_paths.push_back(assembly_filename);

    return cc.run();
}

auto binrec::elf_exe_to_obj(StringRef output_path, LinkContext &ctx) -> ErrorOr<vector<SectionInfo>>
{
    auto section_files_or_err = copy_sections(ctx);
    if (error_code ec = section_files_or_err.getError()) {
        return ec;
    }
    auto &section_files = section_files_or_err.get();

    if (error_code ec = make_object(section_files, output_path, ctx.work_dir)) {
        return ec;
    }

    vector<SectionInfo> sections;
    transform(
        section_files.begin(),
        section_files.end(),
        back_inserter(sections),
        [](const SectionFile &file) { return file.section; });
    return sections;
}
