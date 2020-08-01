#include <string>
#include <assert.h>
#include <string.h>

#include "PEReader.h"

using namespace std;

PEReader::PEReader(const char *path) : infile(path, ifstream::binary) {
    assert(readBuf(&DOSHeader, 0, sizeof(DOSHeader)));
    assert(readBuf(&NTHeader, DOSHeader.e_lfanew, sizeof(NTHeader)));
    sectionsInited = false;
}

const SectionTable &PEReader::getSectionTable() {
    if (!sectionsInited) {
        unsigned offset = DOSHeader.e_lfanew + sizeof(IMAGE_NT_SIGNATURE) +
            sizeof(IMAGE_FILE_HEADER) + sizeof(IMAGE_OPTIONAL_HEADER);

        sections.resize(NTHeader.FileHeader.NumberOfSections);

        for (auto it = sections.begin(); it != sections.end(); it++) {
            Section &section = *it;
            IMAGE_SECTION_HEADER hdr;

            assert(readBuf(&hdr, offset, sizeof(hdr)));

            section.name = (const char*)hdr.Name;
            section.offset = hdr.PointerToRawData;
            section.rawSize = hdr.SizeOfRawData;
            section.virtualSize = hdr.Misc.VirtualSize;
            section.loadBase = hdr.VirtualAddress;
            section.flags = hdr.Characteristics;

            offset += sizeof(hdr);
        }

        sectionsInited = true;
    }

    return sections;
}

PEReader::~PEReader() {
    infile.close();
}

bool PEReader::readBuf(void *buf, unsigned offset, unsigned size) {
    infile.seekg(offset, infile.beg);
    infile.read((char*)buf, size);
    return !!infile;
}

const Section *PEReader::findSectionByName(const char *name) {
    getSectionTable();

    for (SectionTable::iterator it = sections.begin(); it != sections.end(); it++) {
        const Section &section = *it;

        if (section.name == name)
            return &section;
    }

    return NULL;
}

uint8_t *PEReader::readSection(const Section &section) {
    uint8_t *buf = new uint8_t[section.virtualSize];

    if (!readBuf(buf, section.offset, section.virtualSize)) {
        delete [] buf;
        return NULL;
    }

    return buf;
}
