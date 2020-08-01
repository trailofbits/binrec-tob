#ifndef _SECTION_UTILS_H
#define _SECTION_UTILS_H

#include "llvm/IR/Module.h"
#include "llvm/IR/GlobalVariable.h"

#define WRAPPER_SECTION ".wrapper"
#define SECTIONS_METADATA "sections"

using namespace llvm;

typedef struct {
    MDNode *md;
    std::string name;
    size_t loadBase;
    size_t size;
    size_t fileOffset;
    GlobalVariable *global;
    void *param;
} section_meta_t;

/*
 * Type of function mapped by mapToSection
 * Returns true if iteration should stop, false if iteration should continue to
 * next section
 */
typedef bool (*section_map_fn_t)(section_meta_t &s, void *param);

StringRef getSourcePath(Module &m);

void writeSectionConfig(StringRef name, size_t loadBase);

void copySection(Module &m, section_meta_t &s, uint8_t *data, bool readonly);

void readSectionMeta(section_meta_t &s, MDNode *md);

void writeSectionMeta(Module &m, section_meta_t &s);

/*
 * Map function to all sections in "sections" metadata
 * Returns true if iteration ended by `true` return value of iteration
 * function, false otherwise
 */
bool mapToSections(Module &m, section_map_fn_t fn, void *param);

bool readFromBinary(Module &m, void *buf, unsigned offset, unsigned size);

bool findSectionByName(Module &m, const std::string name, section_meta_t &s);

#endif  // _SECTION_UTILS_H
