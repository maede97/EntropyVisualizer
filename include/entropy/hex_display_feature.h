#pragma once

#include <cstdint>
#include <string>
#include <vector>

#ifndef RGBA_COLOR
#define RGBA_COLOR(R, G, B, A)                                                 \
    (((unsigned int)(A) << 24) | ((unsigned int)(B) << 0) |                    \
     ((unsigned int)(G) << 8) | ((unsigned int)(R) << 16))
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
    virtual std::string getVersion() const = 0;
    virtual std::string getAuthor() const = 0;
    virtual int getPriority() const = 0; // for overlapping highlights, higher
                                         // priority takes precedence
    virtual std::vector<Highlight>
    getHighlights(const std::vector<uint8_t> &sectorData,
                  size_t sectorIndex) const = 0;
};

} // namespace entropy