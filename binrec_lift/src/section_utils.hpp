#ifndef BINREC_SECTION_UTILS_HPP
#define BINREC_SECTION_UTILS_HPP

#include <cstddef>
#include <llvm/IR/GlobalVariable.h>
#include <llvm/IR/Module.h>

#define WRAPPER_SECTION ".wrapper"
#define SECTIONS_METADATA "sections"

using namespace llvm;

struct section_meta_t {
    MDNode *md;
    std::string name;
    size_t loadBase;
    size_t size;
    size_t fileOffset;
    GlobalVariable *global;
    void *param;
};

/*
 * Type of function mapped by mapToSection
 * Returns true if iteration should stop, false if iteration should continue to
 * next section
 */
using section_map_fn_t = bool (*)(section_meta_t &, void *);

auto getSourcePath(Module &m) -> StringRef;

void writeSectionConfig(StringRef name, size_t loadBase);

void copySection(Module &m, section_meta_t &s, std::byte *data, bool readonly);

void readSectionMeta(section_meta_t &s, MDNode *md);

void writeSectionMeta(Module &m, section_meta_t &s);

/*
 * Map function to all sections in "sections" metadata
 * Returns true if iteration ended by `true` return value of iteration
 * function, false otherwise
 */
auto mapToSections(Module &m, section_map_fn_t fn, void *param) -> bool;

auto readFromBinary(Module &m, void *buf, unsigned offset, unsigned size) -> bool;

auto findSectionByName(Module &m, const std::string name, section_meta_t &s) -> bool;

#endif
