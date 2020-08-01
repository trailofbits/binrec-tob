#ifndef _PE_READER_H
#define _PE_READER_H

#include <iostream>
#include <fstream>
#include <vector>

#include "nt-headers.h"

using namespace std;

struct Section {
    string name;
    unsigned offset;
    unsigned rawSize;
    unsigned virtualSize;
    unsigned loadBase;
    unsigned flags;
};

typedef vector<Section> SectionTable;

struct PEReader {
    PEReader(const char *path);
    ~PEReader();

    bool readBuf(void *buf, unsigned offset, unsigned size);

    const SectionTable &getSectionTable();
    const Section *findSectionByName(const char *name);
    uint8_t *readSection(const Section &section);

private:
    ifstream infile;

    IMAGE_DOS_HEADER DOSHeader;
    IMAGE_NT_HEADERS NTHeader;

    SectionTable sections;
    bool sectionsInited;
};

#endif  // _PE_READER_H
