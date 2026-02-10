#include <algorithm>
#include <entropy/app.h>
#include <entropy/cache.h>
#include <entropy/icon.h>
#include <entropy/version.h>
#include <iostream>
#include <string>

namespace entropy {

#define ALL_FILES_FILTER "All files {((.*))}"

void addToRecentFiles(AppState &state, const std::string &cache_file, const std::string &file_path) {
    const std::string cache_ext = ".cache.bin";
    if (cache_file.size() < cache_ext.size() || cache_file.substr(cache_file.size() - cache_ext.size()) != cache_ext) {
        return;
    }

    auto pair = std::make_pair(cache_file, file_path);

    // Remove if already exists (to move to front)
    auto it = std::find(state.recentCacheFiles.begin(), state.recentCacheFiles.end(), pair);
    if (it != state.recentCacheFiles.end()) {
        state.recentCacheFiles.erase(it);
    }
    // Add to front
    state.recentCacheFiles.insert(state.recentCacheFiles.begin(), pair);
    // Keep only 10 most recent
    if (state.recentCacheFiles.size() > 10) {
        state.recentCacheFiles.pop_back();
    }
}

void AppState::resetHexDisplayGradientColors() {
    gradient_widget.gradient().clear();
    gradient_widget.gradient().add_mark(ImGG::Mark{ImGG::RelativePosition{0.0f}, ImVec4(0.0f, 0.0f, 1.0f, 1.0f)});
    gradient_widget.gradient().add_mark(ImGG::Mark{ImGG::RelativePosition{0.25f}, ImVec4(0.0f, 1.0f, 1.0f, 1.0f)});
    gradient_widget.gradient().add_mark(ImGG::Mark{ImGG::RelativePosition{0.5f}, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)});
    gradient_widget.gradient().add_mark(ImGG::Mark{ImGG::RelativePosition{0.75f}, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)});
    gradient_widget.gradient().add_mark(ImGG::Mark{ImGG::RelativePosition{1.0f}, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)});

    gradient_widget.gradient().interpolation_mode() = ImGG::Interpolation::Linear;
}

int parseCommandLine(int argc, char **argv, AppState &state) {
    state.originalFile = ""; // Reset
    if (argc == 3) {
        // Two arguments: cache file and source file
        std::string cache_path = argv[1];
        std::string source_path = argv[2];

        const std::string cache_ext = ".cache.bin";
        if (cache_path.size() >= cache_ext.size() && cache_path.substr(cache_path.size() - cache_ext.size()) == cache_ext) {
            state.lastCacheFile = cache_path;
            state.originalFile = source_path;
            state.file.open(cache_path, std::ios::binary);
            if (!state.file) {
                std::cerr << "Failed to open cache file!\n";
                return 1;
            }

            state.file.seekg(0, std::ios::end);
            state.file_size = (size_t)state.file.tellg();
            state.file.seekg(0, std::ios::beg);
            if (state.file_size == 0) {
                std::cerr << "Error: cache file is empty.\n";
                return 1;
            }

            state.all_cache_data.resize(state.file_size);
            state.file.read(reinterpret_cast<char *>(state.all_cache_data.data()), state.file_size);

            state.redrawBlock = true;

            addToRecentFiles(state, cache_path, source_path);
        } else {
            std::cerr << "First argument must be a .cache.bin file when "
                         "providing two arguments.\n";
            return 1;
        }
    } else if (argc == 2) {
        // One argument: either cache or source
        std::string arg_path = argv[1];

        const std::string cache_ext = ".cache.bin";
        if (arg_path.size() >= cache_ext.size() && arg_path.substr(arg_path.size() - cache_ext.size()) == cache_ext) {
            // Open cache file directly
            state.lastCacheFile = arg_path;
            state.file.open(arg_path, std::ios::binary);
            if (!state.file) {
                std::cerr << "Failed to open file!\n";
                return 1;
            }

            state.file.seekg(0, std::ios::end);
            state.file_size = (size_t)state.file.tellg();
            state.file.seekg(0, std::ios::beg);
            if (state.file_size == 0) {
                std::cerr << "Error: file is empty.\n";
                return 1;
            }

            state.all_cache_data.resize(state.file_size);
            state.file.read(reinterpret_cast<char *>(state.all_cache_data.data()), state.file_size);

            state.redrawBlock = true;
            state.promptForSource = true;

            addToRecentFiles(state, state.lastCacheFile, "");
        } else {
            // Start cache generation for the provided source file (background
            // thread)
            state.showCacheGen = true;
            cacheDone = false;
            cacheFailed = false;

            cacheProgress = 0;

            std::string filename_only = std::filesystem::path(arg_path).filename().string();
            std::string cache_file = (std::filesystem::temp_directory_path() / (filename_only + ".cache.bin")).string();
            state.lastCacheFile = cache_file;
            state.originalFile = arg_path;

            state.cacheThread = std::thread([arg_path, cache_file]() { generateCacheThreaded(arg_path, cache_file); });
            state.cacheThread.detach();

            addToRecentFiles(state, cache_file, arg_path);
        }
    }
    return 0;
}

void handleDroppedFiles(AppState &state, UiState &uiState, std::function<void(size_t)> loadHexData) {
    if (!state.droppedFiles.empty()) {
        for (const auto &path : state.droppedFiles) {
            // Decide what to do based on extension
            const std::string cache_ext = ".cache.bin";
            if (path.size() >= cache_ext.size() && path.substr(path.size() - cache_ext.size()) == cache_ext) {
                state.lastCacheFile = path;
                state.originalFile = ""; // Reset

                state.file.close();
                state.file.open(path, std::ios::binary);
                if (state.file) {
                    state.file.seekg(0, std::ios::end);
                    state.file_size = (size_t)state.file.tellg();
                    state.file.seekg(0, std::ios::beg);
                    state.all_cache_data.resize(state.file_size);
                    state.file.read(reinterpret_cast<char *>(state.all_cache_data.data()), state.file_size);
                    state.current_block = 0;
                    state.block_slider = 0;
                    state.redrawBlock = true;
                    if (state.originalFile.empty()) {
                        state.promptForSource = true;
                    }
                    loadHexData(0);
                    addToRecentFiles(state, path, "");
                }
            } else {
                state.showCacheGen = true;
                cacheDone = false;
                cacheFailed = false;
                cacheProgress = 0;

                std::string filename_only = fs::path(path).filename().string();
                std::string cacheFile = (fs::temp_directory_path() / (filename_only + ".cache.bin")).string();

                state.lastCacheFile = cacheFile;
                state.originalFile = path;

                state.cacheThread = std::thread([path, cacheFile]() { generateCacheThreaded(path, cacheFile); });
                state.cacheThread.detach();

                // Clear old visualization
                state.file.close();
                state.all_cache_data.clear();
                state.file_size = 0;
                state.current_block = 0;
                state.block_slider = 0;
                state.redrawBlock = true;

                // Clear hex view
                uiState.highlighted_sector = SIZE_MAX;
                uiState.currentSectorData.clear();
                uiState.currentSectorIndex = 0;

                addToRecentFiles(state, cacheFile, path);
            }
        }

        state.droppedFiles.clear();
    }
}

void handleKeyboardShortcuts(AppState &state, UiState &uiState, IGFD::FileDialogConfig &config, GLFWwindow *window,
                             std::function<void(size_t)> loadHexData) {
    if (ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        if (ImGuiFileDialog::Instance()->IsOpened()) {
            ImGuiFileDialog::Instance()->Close();
        } else {
            glfwSetWindowShouldClose(window, GLFW_TRUE);
        }
    }

    ImGuiIO &io = ImGui::GetIO();
    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_F)) {
        uiState.showSearchWindow = true;
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O)) {
        // CTRL+ O (open)
        config.path = fs::temp_directory_path().string();
        ImGuiFileDialog::Instance()->OpenDialog("OpenCacheDlg", "Open Cache File", ".cache.bin", config);
    }

    if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_G)) {
        config.path = "";
        ImGuiFileDialog::Instance()->OpenDialog("GenerateFileDlg", "Select Source File", ALL_FILES_FILTER, config);
    }

    // Handle arrow keys for moving selected sector
    if (uiState.highlighted_sector != SIZE_MAX) {
        size_t block_size = state.block_width * state.block_height;
        bool moved = false;
        if (ImGui::IsKeyPressed(ImGuiKey_LeftArrow) && uiState.highlighted_sector > 0) {
            uiState.highlighted_sector--;
            moved = true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_RightArrow) && uiState.highlighted_sector < state.all_cache_data.size() - 1) {
            uiState.highlighted_sector++;
            moved = true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_UpArrow) && uiState.highlighted_sector >= state.block_width) {
            uiState.highlighted_sector -= state.block_width;
            moved = true;
        } else if (ImGui::IsKeyPressed(ImGuiKey_DownArrow) && uiState.highlighted_sector + state.block_width < state.all_cache_data.size()) {
            uiState.highlighted_sector += state.block_width;
            moved = true;
        }
        if (moved) {
            size_t new_block = uiState.highlighted_sector / block_size;
            if (new_block != state.current_block) {
                state.current_block = new_block;
                state.block_slider = state.current_block;
                state.redrawBlock = true;
            }
            loadHexData(uiState.highlighted_sector);
        }
    }

    // n, p move to next/previous block
    if (ImGui::IsKeyPressed(ImGuiKey_N)) {
        size_t block_size = state.block_width * state.block_height;
        size_t num_blocks = (state.file_size + block_size - 1) / block_size;
        if (state.current_block + 1 < num_blocks) {
            state.current_block++;
            state.block_slider = (int)state.current_block;
            state.redrawBlock = true;
        }
    }

    if (ImGui::IsKeyPressed(ImGuiKey_P)) {
        if (state.current_block > 0) {
            state.current_block--;
            state.block_slider = (int)state.current_block;
            state.redrawBlock = true;
        }
    }
}

void updateAutoplay(AppState &state) {
    if (state.autoplay) {
        double now = glfwGetTime();
        if (now - state.last_autoplay_time >= state.autoplay_interval) {
            state.last_autoplay_time = now;
            size_t block_size = state.block_width * state.block_height;
            size_t num_blocks = (state.file_size + block_size - 1) / block_size;
            if (state.current_block + 1 < num_blocks) {
                state.current_block++;
                state.block_slider = (int)state.current_block;
                state.redrawBlock = true;
            } else {
                state.autoplay = false;
            }
        }
    }
}

void initializeWindowAndGL(GLFWwindow *&window, GLuint &tex) {
    if (!glfwInit())
        return;

    const char *glsl_version = "#version 130";
    window = glfwCreateWindow(1800, 1200, (std::string("Entropy Visualizer ") + std::string(VERSION)).c_str(), nullptr, nullptr);
    if (!window)
        return;

    GLFWimage icon;
    icon.width = ICON_WIDTH;
    icon.height = ICON_HEIGHT;
    icon.pixels = const_cast<unsigned char *>(ICON_PIXELS);
    glfwSetWindowIcon(window, 1, &icon);

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = "entropy_visualizer.ini";
    io.LogFilename = nullptr;

    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

    (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}

void mainLoop(GLFWwindow *window, GLuint tex, AppState &state, UiState &uiState, IGFD::FileDialogConfig &config,
              std::function<void(size_t)> loadHexData) {
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);

        size_t block_size = state.block_width * state.block_height;

        // Handle dropped files
        handleDroppedFiles(state, uiState, loadHexData);

        // Handle keyboard shortcuts
        handleKeyboardShortcuts(state, uiState, config, window, loadHexData);

        // Zoom and pan
        const float zoom_min = 0.25f;
        const float zoom_max = 20.0f;
        ImVec2 mouse_pos = ImGui::GetIO().MousePos;
        float wheel = ImGui::GetIO().MouseWheel;
        if (wheel != 0.0f && !ImGui::GetIO().WantCaptureMouse) {
            float old_zoom = state.zoom;
            state.zoom = clip(state.zoom + wheel * 0.20f * state.zoom, zoom_min, zoom_max);
            state.pan_offset.x = (state.pan_offset.x + mouse_pos.x) * (state.zoom / old_zoom) - mouse_pos.x;
            state.pan_offset.y = (state.pan_offset.y + mouse_pos.y) * (state.zoom / old_zoom) - mouse_pos.y;
        }

        if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
            ImVec2 delta = ImGui::GetIO().MouseDelta;
            state.pan_offset.x -= delta.x;
            state.pan_offset.y -= delta.y;
        }

        if (state.redrawBlock) {
            if (load_block_from_file(state.file, state.current_block, block_size, state.file_size, state.block_buffer)) {
                upload_block(tex, state.block_buffer, state.block_width, state.block_height, state);
            } else {
                // Clear visualization if load fails
                state.block_buffer.assign(0, 0);
                upload_block(tex, state.block_buffer, state.block_width, state.block_height, state);
            }
            state.redrawBlock = false;
        }

        // Menu bar
        if (ImGui::BeginMainMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Generate Cache")) {
                    ImGuiFileDialog::Instance()->OpenDialog("GenerateFileDlg", "Select Source File", ALL_FILES_FILTER, config);
                }
                if (ImGui::MenuItem("Open Cache")) {
                    ImGuiFileDialog::Instance()->OpenDialog("OpenCacheDlg", "Open Cache File", ".cache.bin", config);
                }

                if (ImGui::MenuItem("Find")) {
                    uiState.showSearchWindow = true;
                }
                
                // Recent files submenu
                if (!state.recentCacheFiles.empty() && ImGui::BeginMenu("Recent Cache Files")) {
                    for (const auto &filePair : state.recentCacheFiles) {
                        std::string menuItemText;
                        if (filePair.second.length() > 0)
                            menuItemText = filePair.first + " | " + filePair.second;
                        else
                            menuItemText = filePair.first;
                        if (ImGui::MenuItem(menuItemText.c_str())) {
                            // Open the selected recent cache file
                            state.lastCacheFile = filePair.first;
                            state.originalFile = filePair.second;
                            state.file.close();
                            state.file.open(filePair.first, std::ios::binary);
                            if (state.file) {
                                state.file.seekg(0, std::ios::end);
                                state.file_size = (size_t)state.file.tellg();
                                state.file.seekg(0, std::ios::beg);
                                state.all_cache_data.resize(state.file_size);
                                state.file.read(reinterpret_cast<char *>(state.all_cache_data.data()), state.file_size);
                                state.current_block = 0;
                                state.block_slider = 0;
                                state.redrawBlock = true;
                                if (state.originalFile.empty()) {
                                    state.promptForSource = true;
                                }

                                loadHexData(0);
                                addToRecentFiles(state, filePair.first, filePair.second);
                            }
                        }
                    }
                    ImGui::EndMenu();
                }
                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Misc")) {
                if (ImGui::MenuItem("About")) {
                    uiState.showAboutUs = true;
                }
                if (ImGui::MenuItem("Help")) {
                    uiState.showHelp = true;
                }
                if (ImGui::MenuItem("Exit")) {
                    glfwSetWindowShouldClose(window, GLFW_TRUE);
                }
                ImGui::EndMenu();
            }
            if (ImGui::BeginMenu("Settings")) {
                if (ImGui::BeginMenu("Enabled Features")) {
                    for (const auto *feature : state.hexDisplayFeatureManager->getFeatures()) {
                        ImGui::MenuItem(feature->getName().c_str(), NULL, &state.featureEnabled.at(feature->getName()));
                    }
                    ImGui::EndMenu();
                }

                if (ImGui::MenuItem("Feature Settings")) {
                    uiState.showFeatureSettings = true;
                }

                if (ImGui::MenuItem("General Settings")) {
                    uiState.showGeneralSettings = true;
                }

                ImGui::EndMenu();
            }
        }
        ImGui::EndMainMenuBar();

        // File dialogs
        handleFileDialogs(uiState, state, config, loadHexData);

        // Cache generation window
        if (state.showCacheGen) {
            if (!cacheDone) {
                float p = (cacheTotal > 0) ? float(cacheProgress) / float(cacheTotal) : 0.0f;
                ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);
                ImGui::Begin("Cache Generation");
                ImGui::Text("Generating cache...");
                ImGui::ProgressBar(p, ImVec2(-1, 0));
                ImGui::Text("%zu / %zu sectors", (size_t)cacheProgress, cacheTotal);
                ImGui::End();
            } else {
                ImGui::SetNextWindowSize(ImVec2(400, 150), ImGuiCond_FirstUseEver);
                ImGui::Begin("Cache Generation");
                if (cacheFailed) {
                    ImGui::Text("Error generating cache file!");
                } else {
                    state.file.close();
                    state.file.open(state.lastCacheFile, std::ios::binary);
                    if (state.file) {
                        state.file.seekg(0, std::ios::end);
                        state.file_size = (size_t)state.file.tellg();
                        state.file.seekg(0, std::ios::beg);
                        state.all_cache_data.resize(state.file_size);
                        state.file.read(reinterpret_cast<char *>(state.all_cache_data.data()), state.file_size);
                        state.current_block = 0;
                        state.block_slider = 0;
                        state.redrawBlock = true;
                        loadHexData(0);
                    }
                    state.showCacheGen = false;
                }
                if (ImGui::Button("Close")) {
                    state.showCacheGen = false;
                }
                ImGui::End();
            }
        }

        ImGui::Begin("Controls");

        size_t num_blocks = (state.file_size + block_size - 1) / block_size;

        if (num_blocks > 0) {
            ImGui::Text("Block: %zu / %zu", state.current_block + 1, num_blocks);
            int slider_val = (int)state.block_slider;
            if (ImGui::SliderInt("Block", &slider_val, 0, (int)num_blocks - 1)) {
                state.block_slider = (size_t)slider_val;
                state.current_block = (size_t)slider_val;
                state.redrawBlock = true;
            }
        }

        ImGui::Separator();

        ImGui::Text("Zoom: %.2f", state.zoom);
        ImGui::SliderFloat("Zoom", &state.zoom, 0.25f, 20.0f, "%.2f");

        ImGui::Separator();

        ImGui::Text("Pan Offset: (%.1f, %.1f)", state.pan_offset.x, state.pan_offset.y);
        ImGui::InputFloat("Pan X", &state.pan_offset.x);
        ImGui::InputFloat("Pan Y", &state.pan_offset.y);

        ImGui::Separator();

        if (ImGui::Checkbox("Autoplay", &state.autoplay)) {
            // Autoplay toggled
        }
        if (state.autoplay) {
            ImGui::InputFloat("Interval (s)", &state.autoplay_interval, 0.1f, 1.0f, "%.1f");
        }

        ImGui::Separator();

        if (uiState.highlighted_sector != SIZE_MAX) {
            ImGui::Text("Highlighted Sector: %zu", uiState.highlighted_sector);
            if (ImGui::Button("Go to Highlighted")) {
                size_t block = uiState.highlighted_sector / block_size;
                state.current_block = block;
                state.block_slider = (int)block;
                state.redrawBlock = true;
                loadHexData(uiState.highlighted_sector);
            }
            if (ImGui::Button("Clear Highlight")) {
                uiState.highlighted_sector = SIZE_MAX;
                uiState.currentSectorData.clear();
                uiState.currentSectorIndex = 0;
            }
        }

        ImGui::Separator();

        if (ImGui::Button("Reset View")) {
            state.zoom = 2.f;
            state.pan_offset = ImVec2(-50.0f, -50.0f);
        }

        if (state.promptForSource) {
            if (ImGui::Button("Select Source File")) {
                IGFD::FileDialogConfig config;
                config.path = "";
                ImGuiFileDialog::Instance()->OpenDialog("SelectSourceDlg", "Select Source File", ALL_FILES_FILTER, config);
            }
        }
        if (!state.showCacheGen && !state.originalFile.empty()) {
            ImGui::Text("Source: %s", state.originalFile.c_str());
        }
        if (!state.showCacheGen && !state.lastCacheFile.empty()) {
            ImGui::Text("Cache: %s", state.lastCacheFile.c_str());
        }

        ImGui::End();

        // UI Windows
        renderAboutWindow(uiState);
        renderHelpWindow(uiState);
        renderHexViewWindow(uiState, state);
        renderSearchWindow(uiState, state, loadHexData);

        if (uiState.showFeatureSettings) {
            ImGui::Begin("Feature Settings", &uiState.showFeatureSettings);
            for (const auto *feature : state.hexDisplayFeatureManager->getFeatures()) {
                ImGui::Text("%s", feature->getName().c_str());
                feature->renderSettingsPanel();
                ImGui::Separator();
            }
            ImGui::End();
        }

        if (uiState.showGeneralSettings) {
            ImGui::Begin("General Settings", &uiState.showGeneralSettings);

            ImGG::Settings settings;
            settings.flags = ImGG::Flag::NoResetButton;
            settings.gradient_width = 5000; // arbitrary large to use full width
            if (state.gradient_widget.widget("Hex Color Gradient", settings)) {
                state.redrawBlock = true;
            }
            if (ImGG::interpolation_mode_widget("Interpolation Mode", &state.gradient_widget.gradient().interpolation_mode())) {
                state.redrawBlock = true;
            }
            if (ImGui::Button("Reset Gradient to Default")) {
                state.resetHexDisplayGradientColors();
                state.redrawBlock = true;
            }
            ImGui::End();
        }

        // Visualization
        ImDrawList *draw_list = ImGui::GetBackgroundDrawList();
        renderVisualization(draw_list, tex, state.block_buffer, state.zoom, state.pan_offset, state.current_block, block_size, state.block_width,
                            state.block_height, uiState, loadHexData);

        // Update autoplay
        updateAutoplay(state);

        ImGui::Render();
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
    }
}

} // namespace entropy