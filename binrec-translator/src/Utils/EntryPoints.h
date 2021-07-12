#ifndef _ENTRY_POINTS_H
#define _ENTRY_POINTS_H

#include <string>
#include <vector>

struct SymbolInfo {
    std::string name;
    uint64_t vAddr;
};

class EntryPointProvider {
public:
    virtual std::vector<uint64_t> getVisibleEntryPoints() const = 0;
    virtual const SymbolInfo *getInfo(uint64_t address) const = 0;

    virtual ~EntryPointProvider() = default;
};

class EntryPointRepository final : public EntryPointProvider {
    std::vector<const EntryPointProvider *> providers;

public:
    void addProvider(EntryPointProvider &provider);

    std::vector<uint64_t> getVisibleEntryPoints() const override;
    const SymbolInfo *getInfo(uint64_t address) const override;
};

#endif
