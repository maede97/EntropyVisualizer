#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "imgui.h"

#ifndef RGBA_COLOR
#define RGBA_COLOR(R, G, B, A) (((unsigned int)(A) << 24) | ((unsigned int)(B) << 0) | ((unsigned int)(G) << 8) | ((unsigned int)(R) << 16))
#endif

namespace entropy {

struct Highlight {
    size_t offset;  // relative to the data start
    uint32_t color; // ImGui color, like IM_COL32(r,g,b,a)
};

class HexDisplayFeature {
  public:
    virtual ~HexDisplayFeature() = default;
    virtual std::string getName() const = 0;
    virtual std::string getSlug() const = 0;
    virtual std::string getVersion() const = 0;
    virtual std::string getAuthor() const = 0;
    virtual int getPriority() const = 0; // for overlapping highlights, higher
                                         // priority takes precedence
    virtual std::vector<Highlight> getHighlights(const std::vector<uint8_t> &sectorData, size_t sectorIndex) const = 0;
    virtual void renderSettingsPanel() const {
        ImVec4 col = ImGui::ColorConvertU32ToFloat4(color);
        ImGui::PushID(getName().c_str());
        if (ImGui::ColorEdit4("Color", (float *)&col)) {
            color = ImGui::ColorConvertFloat4ToU32(col);
        }
        ImGui::PopID();
    }
    virtual void setConfig(const std::string &key, const std::string &value) {
        if (key == "color") {
            unsigned int color_value = 0;
            if (sscanf(value.c_str(), "0x%X", &color_value) == 1) {
                color = color_value;
            }
        }
    }

    virtual std::string getConfigValue(const std::string &key) const {
        if (key == "color") {
            char buf[11];
            snprintf(buf, sizeof(buf), "0x%08X", color);
            return std::string(buf);
        }
        return "";
    }

    virtual std::vector<std::string> getConfigOptions() const { return {"color"}; }

  public:
    mutable uint32_t color = IM_COL32(255, 0, 0, 255); // Default red color
};

} // namespace entropy