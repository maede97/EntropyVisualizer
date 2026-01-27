#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <entropy/hex_display_feature.h>

namespace entropy {

class ZeroBytesFeature : public HexDisplayFeature {
  public:
    ZeroBytesFeature() { color = IM_COL32(255, 255, 255, 100); }
    std::string getName() const override { return "Zero Bytes Finder"; }
    std::string getSlug() const override { return "zeros"; }
    std::string getVersion() const override { return "1.0"; }
    std::string getAuthor() const override { return "Entropy Visualizer Team"; }
    int getPriority() const override { return 1; }

    std::vector<Highlight> getHighlights(const std::vector<uint8_t> &sectorData, size_t sectorIndex) const override {
        std::vector<Highlight> highlights;
        for (size_t i = 0; i < sectorData.size(); i++) {
            if (sectorData[i] == 0) {
                highlights.push_back({i, this->color});
            }

            if (colorFF && sectorData[i] == 0xFF) {
                highlights.push_back({i, this->color});
            }
        }
        return highlights;
    }

    void renderSettingsPanel() const override {
        ImGui::PushID(getName().c_str());

        // Default Config (color)
        HexDisplayFeature::renderSettingsPanel();

        bool changed = ImGui::Checkbox("Color FF as well", &colorFF);
        if (changed) {
            highlightCache.clear();
        }

        ImGui::PopID();
    }

    std::vector<std::string> getConfigOptions() const override {
        std::vector<std::string> options = HexDisplayFeature::getConfigOptions(); // default options
        options.push_back("colorFF");
        return options;
    }

    std::string getConfigValue(const std::string &key) const override {
        if (key == "colorFF") {
            return colorFF ? "1" : "0";
        }
        // default options
        return HexDisplayFeature::getConfigValue(key);
    }

    void setConfig(const std::string &key, const std::string &value) override {
        if (key == "colorFF") {
            colorFF = (value == "1");
            highlightCache.clear();
            return;
        }
        // default options
        HexDisplayFeature::setConfig(key, value);
    }

  private:
    mutable bool colorFF = false;
};

} // namespace entropy
