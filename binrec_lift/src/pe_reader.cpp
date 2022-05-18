#include "pe_reader.hpp"
#include "error.hpp"
#include <cassert>
#include <cstring>
#include <string>

#define PASS_NAME "pe_reader"
#define PASS_ASSERT(cond) LIFT_ASSERT(PASS_NAME, cond)

using namespace std;
using namespace llvm;

PeReader::PeReader(const char *path) : m_infile(path, ifstream::binary)
{
    PASS_ASSERT(readBuf(&m_dosHeader, 0, sizeof(m_dosHeader)));
    PASS_ASSERT(readBuf(&m_ntHeader, m_dosHeader.AddressOfNewExeHeader + 4, sizeof(m_ntHeader)));
    m_sectionsInited = false;
}

auto PeReader::getSectionTable() -> const SectionTable &
{
    if (!m_sectionsInited) {
        unsigned offset = m_ntHeader.PointerToSymbolTable;

        m_sections.resize(m_ntHeader.NumberOfSections);

        for (auto &section : m_sections) {
            object::coff_section hdr{};

            PASS_ASSERT(readBuf(&hdr, offset, sizeof(hdr)));

            section.name = (const char *)hdr.Name;
            section.offset = hdr.PointerToRawData;
            section.rawSize = hdr.SizeOfRawData;
            section.virtualSize = hdr.VirtualSize;
            section.loadBase = hdr.VirtualAddress;
            section.flags = hdr.Characteristics;

            offset += sizeof(hdr);
        }

        m_sectionsInited = true;
    }

    return m_sections;
}

PeReader::~PeReader()
{
    m_infile.close();
}

auto PeReader::readBuf(void *buf, unsigned offset, unsigned size) -> bool
{
    m_infile.seekg(offset, m_infile.beg);
    m_infile.read((char *)buf, size);
    return !!m_infile;
}

auto PeReader::findSectionByName(const char *name) -> const Section *
{
    getSectionTable();

    for (auto &section : m_sections) {
        if (section.name == name)
            return &section;
    }

    return nullptr;
}

auto PeReader::readSection(const Section &section) -> std::optional<std::vector<std::byte>>
{
    std::vector<std::byte> buf(section.virtualSize);

    if (!readBuf(buf.data(), section.offset, section.virtualSize)) {
        return {};
    }

    return buf;
}
