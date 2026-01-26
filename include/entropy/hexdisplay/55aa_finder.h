#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <entropy/hex_display_feature.h>

namespace entropy {

class AA55FinderFeature : public HexDisplayFeature {
  public:
    AA55FinderFeature() { color = RGBA_COLOR(255, 255, 0, 255); }
    std::string getName() const override { return "55AA Finder"; }
    std::string getSlug() const override { return "55aa"; }
    std::string getVersion() const override { return "1.0"; }
    std::string getAuthor() const override { return "Entropy Visualizer Team"; }
    int getPriority() const override { return 1; }

    std::vector<Highlight> getHighlights(const std::vector<uint8_t> &sectorData, size_t sectorIndex) const override {
        std::vector<Highlight> highlights;
        // Scan for 0x55 0xAA at sector end (510-511)
        if (sectorData.size() >= 512 && sectorData[510] == 0x55 && sectorData[511] == 0xAA) {
            highlights.push_back({510, this->color});
            highlights.push_back({511, this->color});
        }
        return highlights;
    }
};

} // namespace entropy
