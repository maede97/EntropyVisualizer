#include <algorithm>
#include <cmath>
#include <entropy/core.h>

namespace entropy {

double unpack_value(uint8_t packed) {
    uint8_t int_part = packed >> 5;
    uint8_t frac_part = packed & 0x1F;
    return int_part + (frac_part / 32.0);
}

uint8_t pack_value(double value) {
    value = clip(value, 0.0, 7.99);
    uint8_t int_part = static_cast<uint8_t>(value);
    uint8_t frac_part = static_cast<uint8_t>(std::round((value - int_part) * 32.0));
    if (frac_part > 31)
        frac_part = 31;
    return (int_part << 5) | frac_part;
}

double shannon_entropy(const unsigned char *data, size_t size) {
    if (size == 0)
        return 0.0;
    size_t freq[256] = {0};
    for (size_t i = 0; i < size; ++i)
        freq[data[i]]++;
    double entropy = 0.0;
    for (int i = 0; i < 256; ++i) {
        if (freq[i] == 0)
            continue;
        double p = double(freq[i]) / size;
        entropy -= p * std::log2(p);
    }
    return entropy;
}

// Map 0-1 value to color (blue -> green -> yellow -> red)
void value_to_color(float v, uint8_t &r, uint8_t &g, uint8_t &b) {
    if (v < 0.25f) {
        float t = v / 0.25f;
        r = 0;
        g = uint8_t(t * 255);
        b = 255;
    } else if (v < 0.5f) {
        float t = (v - 0.25f) / 0.25f;
        r = 0;
        g = 255;
        b = uint8_t((1.0f - t) * 255);
    } else if (v < 0.75f) {
        float t = (v - 0.5f) / 0.25f;
        r = uint8_t(t * 255);
        g = 255;
        b = 0;
    } else {
        float t = (v - 0.75f) / 0.25f;
        r = 255;
        g = uint8_t((1.0f - t) * 255);
        b = 0;
    }
}

} // namespace entropy