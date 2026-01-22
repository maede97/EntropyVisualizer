#pragma once

#include "version.h"
#include <cstdint>
#include <string>

namespace entropy {

const size_t DEFAULT_BLOCK_WIDTH = 256;
const size_t DEFAULT_BLOCK_HEIGHT = 256;

const std::string ABOUT_STRING = "EntropyVisualizer\nMatthias Hüppi, maede97@hotmail.com\nVersion " +
                                 std::string(VERSION) + " - " + std::string(DATE);
const std::string HELP_STRING = R"(
Entropy Visualizer - User Guide

Getting Started:
- Generate a cache file from a source file (File > Generate Cache or Ctrl+G)
- Open an existing cache file (File > Open Cache or Ctrl+O)
- The cache file contains entropy data for quick visualization

Visualization Controls:
- Mouse wheel: Zoom in/out (only when not over UI elements)
- Right mouse drag: Pan the view
- Left click on a sector: Select and view its hex data
- Rulers show coordinates; selected sector is highlighted in pink on rulers too

Navigation:
- Block slider: Jump to specific blocks
- Prev/Next buttons: Navigate blocks
- Autoplay: Automatically advance through blocks
- Block Height/Width sliders: Adjust grid size (resets to block 0)

Sector Selection and Movement:
- Click a sector to select it and open hex view
- Move selected sector with arrow keys on the keyboard

Search:
- Ctrl+F: Open search window to find sectors by entropy range

Other Shortcuts:
- Escape: Close dialogs or exit application
- File > Find: Same as Ctrl+F

Tips:
- Cache files are generated in temp directory by default
- For hex view, select source file if prompted
- Entropy ranges from 0 (low) to ~8 (high), colored blue to red
- Rulers mark every 32 units for reference
)";

double unpack_value(uint8_t packed);
uint8_t pack_value(double value);
template <typename T> T clip(const T &n, const T &lower, const T &upper) {
    return std::max(lower, std::min(n, upper));
}
double shannon_entropy(const unsigned char *data, size_t size);
void value_to_color(float v, uint8_t &r, uint8_t &g, uint8_t &b);

} // namespace entropy
