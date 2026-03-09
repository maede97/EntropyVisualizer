#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <entropy/hex_display_feature.h>

namespace entropy {

class ExecutableDisplayFeature : public HexDisplayFeature {
  public:
    ExecutableDisplayFeature() { color = IM_COL32(255, 255, 0, 255); }
    std::string getName() const override { return "Executable Finder"; }
    std::string getSlug() const override { return "executable"; }
    std::string getVersion() const override { return "1.0"; }
    std::string getAuthor() const override { return "Entropy Visualizer Team"; }
    int getPriority() const override { return 1; }

    std::vector<Highlight> getHighlights(const std::vector<uint8_t> &sectorData, size_t sectorIndex) const override {
        std::vector<Highlight> highlights;
        // Scan for "MZ" at the start of the sector (0-1)
        if (sectorData.size() >= 2 && sectorData[0] == 'M' && sectorData[1] == 'Z') {
            highlights.push_back({0, this->color});
            highlights.push_back({1, this->color});
        }

        // Scan for "ELF" at the start of the sector (0-3)
        if (sectorData.size() >= 4 && sectorData[0] == 0x7F && sectorData[1] == 'E' && sectorData[2] == 'L' && sectorData[3] == 'F') {
            highlights.push_back({0, this->color});
            highlights.push_back({1, this->color});
            highlights.push_back({2, this->color});
            highlights.push_back({3, this->color});
        }

        return highlights;
    }
};

} // namespace entropy
