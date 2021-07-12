#include "ElfExeToObj.h"
#include "CompilerCommand.h"
#include "LinkError.h"
#include <llvm/Object/ELFObjectFile.h>

using namespace binrec;
using namespace llvm;
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

template <class ELFT> auto getRanges(const object::ELFFile<ELFT> &elfExe) -> vector<SectionRange> {
    vector<SectionRange> ranges;

    auto sections = elfExe.sections();
    cantFail(sections.takeError());
    for (const auto &elfSec : *sections) {
        if (elfSec.sh_addr == 0)
            continue;

        Expected<StringRef> secName = elfExe.getSectionName(elfSec);
        if (!secName)
            continue;

        SectionRange range;
        range.offset = elfSec.sh_offset;
        range.length = elfSec.sh_size;

        range.section.name = *secName;
        range.section.virtualAddress = elfSec.sh_addr;
        range.section.flags = elfSec.sh_flags;
        range.section.size = elfSec.sh_size;
        range.section.nobits = elfSec.sh_type == ELF::SHT_NOBITS;

        ranges.push_back(range);
    }

    return ranges;
}

auto copyRanges(const char *exeData, const vector<SectionRange> &ranges, StringRef workDir)
    -> ErrorOr<vector<SectionFile>> {
    vector<SectionFile> sectionFiles;

    for (const auto &range : ranges) {
        SectionFile sf;
        sf.section = range.section;

        if (!range.section.nobits) {
            sf.path = workDir;
            sf.path += "/section-";
            sf.path += range.section.name;
            sf.path += ".bin";

            error_code ec;
            raw_fd_ostream os(sf.path, ec);
            if (ec) {
                return ec;
            }
            os.write(exeData + range.offset, range.length);
        }

        sectionFiles.push_back(sf);
    }

    return sectionFiles;
}

auto copySections(LinkContext &ctx) -> ErrorOr<vector<SectionFile>> {
    object::Binary *exe = ctx.originalBinary.getBinary();
    if (!exe->isELF()) {
        return LinkError::BadElf;
    }

    vector<SectionRange> ranges;
    if (const object::ELF32LEObjectFile *elf32le = dyn_cast<object::ELF32LEObjectFile>(exe)) {
        ranges = getRanges(elf32le->getELFFile());
    } else if (const object::ELF32BEObjectFile *elf32be = dyn_cast<object::ELF32BEObjectFile>(exe)) {
        ranges = getRanges(elf32be->getELFFile());
    } else if (const object::ELF64LEObjectFile *elf64le = dyn_cast<object::ELF64LEObjectFile>(exe)) {
        ranges = getRanges(elf64le->getELFFile());
    } else if (const object::ELF64BEObjectFile *elf64be = dyn_cast<object::ELF64BEObjectFile>(exe)) {
        ranges = getRanges(elf64be->getELFFile());
    } else {
        return LinkError::BadElf;
    }

    const char *exeData = exe->getData().data();
    return copyRanges(exeData, ranges, ctx.workDir);
}

auto sectionFlagsToStr(uint64_t flags) -> string {
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

auto makeObject(const vector<SectionFile> &sectionFiles, StringRef objectFilename, StringRef workDir) -> error_code {
    SmallString<128> assemblyFilename = workDir;
    assemblyFilename.append("/originalSections.S");

    error_code ec;
    raw_fd_ostream assemblyFile(assemblyFilename, ec);
    if (ec) {
        return ec;
    }

    for (const SectionFile &file : sectionFiles) {
        const SectionInfo &section = file.section;

        const char *progbits = section.nobits ? "" : ",@progbits";

        auto normalizedSectionName = section.name;
        replace(normalizedSectionName.begin(), normalizedSectionName.end(), '.', '_');
        replace(normalizedSectionName.begin(), normalizedSectionName.end(), '-', '_');

        assemblyFile << "\t.global binrec_section" << normalizedSectionName << "\n"
                     << "\t.section\t.orig" << section.name << ",\"" << sectionFlagsToStr(section.flags) << "\""
                     << progbits << "\n"
                     << "binrec_section" << normalizedSectionName << ":\n";

        if (section.nobits) {
            assemblyFile << "\t.zero " << section.size << '\n';
        } else {
            assemblyFile << "\t.incbin \"" << file.path << "\"\n";
        }

        assemblyFile << "\n";
    }
    assemblyFile.close();

    CompilerCommand cc{CompilerCommandMode::Compile};
    cc.outputPath() = objectFilename;
    cc.inputPaths().push_back(assemblyFilename);

    return cc.run();
}
} // namespace

auto binrec::elfExeToObj(StringRef outputPath, LinkContext &ctx) -> ErrorOr<vector<SectionInfo>> {
    auto sectionFilesOrErr = copySections(ctx);
    if (error_code ec = sectionFilesOrErr.getError()) {
        return ec;
    }
    auto &sectionFiles = sectionFilesOrErr.get();

    if (error_code ec = makeObject(sectionFiles, outputPath, ctx.workDir)) {
        return ec;
    }

    vector<SectionInfo> sections;
    transform(sectionFiles.begin(), sectionFiles.end(), back_inserter(sections),
              [](const SectionFile &file) { return file.section; });
    return sections;
}
