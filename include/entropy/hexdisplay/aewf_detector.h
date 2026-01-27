#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <entropy/hex_display_feature.h>

#include <iostream>

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

        // .E01 file
        // Signature: EVF 0x09 0x0d 0x0a 0xff 0x00
        const char aewf_signature[] = {'E', 'V', 'F', 0x09, 0x0d, 0x0a, (char)0xff, 0x00};

        // check if sectorData contains the signature
        for (size_t i = 0; i + sizeof(aewf_signature) <= sectorData.size(); i++) {
            if (memcmp(&sectorData[i], aewf_signature, sizeof(aewf_signature)) == 0) {
                for (size_t j = 0; j < sizeof(aewf_signature); j++) {
                    highlights.push_back({i + j, this->color});
                }
                return highlights;
            }
        }

        // .AD1 file
        // Signature: ADSEGMENTEDFILE\0
        const char ad1_signature[] = {'A', 'D', 'S', 'E', 'G', 'M', 'E', 'N', 'T', 'E', 'D', 'F', 'I', 'L', 'E', 0x00};

        for (size_t i = 0; i + sizeof(ad1_signature) <= sectorData.size(); i++) {
            if (memcmp(&sectorData[i], ad1_signature, sizeof(ad1_signature)) == 0) {
                // Found AD1 signature
                for (size_t j = 0; j < sizeof(ad1_signature); j++) {
                    highlights.push_back({i + j, this->color});
                }
                return highlights;
            }
        }

        return highlights;
    }
};

} // namespace entropy
