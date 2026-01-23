#pragma once
#include <entropy/ui.h>

#include <atomic>
#include <entropy/core.h>
#include <entropy/hex_display_feature_manager.h>
#include <filesystem>
#include <fstream>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <thread>
#include <vector>

namespace fs = std::filesystem;

namespace entropy {

extern std::atomic<size_t> cacheProgress;
extern size_t cacheTotal;
extern std::atomic<bool> cacheDone;
extern std::atomic<bool> cacheFailed;

struct UiState {
    bool showHexView = false;
    bool showAboutUs = false;
    bool showHelp = false;
    bool showSearchWindow = false;
    size_t highlighted_sector = SIZE_MAX;
    std::vector<uint8_t> currentSectorData;
    size_t currentSectorIndex = 0;
    bool promptForSource = false;
    // Search window variables
    int search_type = 0; // 0=max, 1=min, 2=range
    float max_value = 8.0f;
    float min_value = 0.0f;
    float range_min = 0.0f;
    float range_max = 8.0f;
};

struct AppState {
    // File handling
    std::ifstream file;
    size_t file_size = 0;
    size_t current_block = 0;
    bool redrawBlock = false;
    std::vector<uint8_t> block_buffer;
    std::vector<uint8_t> all_cache_data;
    std::vector<std::string> droppedFiles;

    // Configuration
    size_t block_width = DEFAULT_BLOCK_WIDTH;
    size_t block_height = DEFAULT_BLOCK_HEIGHT;

    // Cache
    std::string lastCacheFile;
    std::string originalFile;
    std::thread cacheThread;
    bool promptForSource = false;
    bool showCacheGen = false;

    // UI
    float zoom = 2.f;
    ImVec2 pan_offset = ImVec2(-50.0f, -50.0f);
    size_t block_slider = 0;

    // Autoplay
    bool autoplay = false;
    float autoplay_interval = 0.5f;
    double last_autoplay_time = 0.0;

    // Hex Display Features
    std::unique_ptr<HexDisplayFeatureManager> hexDisplayFeatureManager;
    std::map<std::string, bool> featureEnabled;
};

int parseCommandLine(int argc, char **argv, AppState &state);
void handleDroppedFiles(AppState &state, UiState &uiState, std::function<void(size_t)> loadHexData);
void handleKeyboardShortcuts(AppState &state, UiState &uiState, IGFD::FileDialogConfig &config, GLFWwindow *window,
                             std::function<void(size_t)> loadHexData);
void updateAutoplay(AppState &state);
void initializeWindowAndGL(GLFWwindow *&window, GLuint &tex);
void mainLoop(GLFWwindow *window, GLuint tex, AppState &state, UiState &uiState, IGFD::FileDialogConfig &config,
              std::function<void(size_t)> loadHexData);

} // namespace entropy