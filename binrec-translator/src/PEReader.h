#ifndef BINREC_PEREADER_H
#define BINREC_PEREADER_H

#include <fstream>
#include <llvm/Object/COFF.h>
#include <vector>

struct Section {
    std::string name;
    unsigned offset;
    unsigned rawSize;
    unsigned virtualSize;
    unsigned loadBase;
    unsigned flags;
};

using SectionTable = std::vector<Section>;

struct PEReader {
    PEReader(const char *path);
    ~PEReader();

    auto readBuf(void *buf, unsigned offset, unsigned size) -> bool;

    auto getSectionTable() -> const SectionTable &;
    auto findSectionByName(const char *name) -> const Section *;
    auto readSection(const Section &section) -> std::optional<std::vector<std::byte>>;

private:
    std::ifstream m_infile;

    llvm::object::dos_header m_dosHeader{};
    llvm::object::coff_file_header m_ntHeader{};

    SectionTable m_sections;
    bool m_sectionsInited;
};

#endif // BINREC_PEREADER_H
