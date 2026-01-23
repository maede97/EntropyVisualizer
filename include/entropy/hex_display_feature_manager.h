#pragma once

#include <entropy/hex_display_feature.h>
#include <memory>
#include <string>
#include <vector>

namespace entropy {

class HexDisplayFeatureManager {
  public:
    HexDisplayFeatureManager();
    ~HexDisplayFeatureManager() = default;
    void loadFeatures();
    const std::vector<HexDisplayFeature *> &getFeatures() const { return features; }

  private:
    std::vector<HexDisplayFeature *> features;
};

} // namespace entropy