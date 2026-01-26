#include <entropy/hex_display_feature_manager.h>
#include <entropy/hexdisplay/55aa_finder.h>
#include <entropy/hexdisplay/aewf_detector.h>
#include <entropy/hexdisplay/zero_bytes.h>
#include <iostream>

namespace entropy {

HexDisplayFeatureManager::HexDisplayFeatureManager() { loadFeatures(); }

void HexDisplayFeatureManager::loadFeatures() {
    // Clear any existing features
    for (auto *feature : features) {
        delete feature;
    }
    features.clear();

    // Statically load all hex display features
    features.push_back(new ZeroBytesFeature());
    features.push_back(new AA55FinderFeature());
    features.push_back(new AEWFDetector());
}

} // namespace entropy