#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <entropy/hex_display_feature.h>

namespace entropy {

class ZeroBytesFeature : public HexDisplayFeature {
  public:
    std::string getName() const override { return "Zero Bytes Finder"; }
    std::string getVersion() const override { return "1.0"; }
    std::string getAuthor() const override { return "Entropy Visualizer Team"; }
    int getPriority() const override { return 1; }

    std::vector<Highlight> getHighlights(const std::vector<uint8_t> &sectorData,
                                         size_t sectorIndex) const override {
        std::vector<Highlight> highlights;
        for (size_t i = 0; i < sectorData.size(); i++) {
            if (sectorData[i] == 0) {
                highlights.push_back(
                    {i, RGBA_COLOR(255, 255, 255, 100)}); // Gray highlight
            }
        }
        return highlights;
    }
};

} // namespace entropy
