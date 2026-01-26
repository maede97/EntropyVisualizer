#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <entropy/hex_display_feature.h>

namespace entropy {

class AEWFDetector : public HexDisplayFeature {
  public:
    AEWFDetector() { color = IM_COL32(255, 0, 0, 255); }
    std::string getName() const override { return "AEWF Detector"; }
    std::string getSlug() const override { return "aewf"; }
    std::string getVersion() const override { return "1.0"; }
    std::string getAuthor() const override { return "Entropy Visualizer Team"; }
    int getPriority() const override { return 10; }

    std::vector<Highlight> getHighlights(const std::vector<uint8_t> &sectorData, size_t sectorIndex) const override {
        std::vector<Highlight> highlights;
        // Scan for a AEWF signature at sector start (0-3)
        // Signature: EVF 0x09 	0x0d 	0x0a 	0xff 	0x00
        if (sectorData.size() >= 13 && sectorData[0] == 0x45 && sectorData[1] == 0x56 && sectorData[2] == 0x46 && sectorData[3] == 0x09 &&
            sectorData[4] == 0x0d && sectorData[5] == 0x0a && sectorData[6] == 0xff && sectorData[7] == 0x00) {
            for (size_t i = 0; i < 8; i++) {
                highlights.push_back({i, this->color});
            }
        }

        return highlights;
    }
};

} // namespace entropy
