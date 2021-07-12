#include "PEReader.h"
#include <cassert>
#include <cstring>
#include <string>

using namespace std;
using namespace llvm;

PEReader::PEReader(const char *path) : m_infile(path, ifstream::binary) {
    assert(readBuf(&m_dosHeader, 0, sizeof(m_dosHeader)));
    assert(readBuf(&m_ntHeader, m_dosHeader.AddressOfNewExeHeader + 4, sizeof(m_ntHeader)));
    m_sectionsInited = false;
}

auto PEReader::getSectionTable() -> const SectionTable & {
    if (!m_sectionsInited) {
        unsigned offset = m_ntHeader.PointerToSymbolTable;

        m_sections.resize(m_ntHeader.NumberOfSections);

        for (auto &section : m_sections) {
            object::coff_section hdr{};

            assert(readBuf(&hdr, offset, sizeof(hdr)));

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

PEReader::~PEReader() { m_infile.close(); }

auto PEReader::readBuf(void *buf, unsigned offset, unsigned size) -> bool {
    m_infile.seekg(offset, m_infile.beg);
    m_infile.read((char *)buf, size);
    return !!m_infile;
}

auto PEReader::findSectionByName(const char *name) -> const Section * {
    getSectionTable();

    for (auto &section : m_sections) {
        if (section.name == name)
            return &section;
    }

    return nullptr;
}

auto PEReader::readSection(const Section &section) -> std::optional<std::vector<std::byte>> {
    std::vector<std::byte> buf(section.virtualSize);

    if (!readBuf(buf.data(), section.offset, section.virtualSize)) {
        return {};
    }

    return buf;
}
