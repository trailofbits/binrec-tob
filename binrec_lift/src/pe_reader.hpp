#ifndef BINREC_PE_READER_HPP
#define BINREC_PE_READER_HPP

#include <fstream>
#include <llvm/Object/COFF.h>
#include <optional>
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

struct PeReader {
    PeReader(const char *path);
    ~PeReader();

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

#endif
