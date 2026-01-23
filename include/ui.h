#pragma once

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "ImGuiFileDialog.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"
#include "imgui.h"
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/glad.h>

namespace entropy {

struct AppState; // Forward declare
struct UiState; // Forward declare

void renderAboutWindow(UiState &uiState);
void renderHelpWindow(UiState &uiState);
void renderHexViewWindow(UiState &uiState);
void renderSearchWindow(UiState &uiState, AppState &appState, std::function<void(size_t)> loadHexData);
void handleFileDialogs(UiState &uiState, AppState &appState, IGFD::FileDialogConfig &config, std::function<void(size_t)> loadHexData);
void renderVisualization(ImDrawList *draw_list, GLuint tex, const std::vector<uint8_t> &block_buffer, float zoom,
                          ImVec2 pan_offset, size_t current_block, size_t block_size, size_t block_width,
                          size_t block_height, UiState &uiState, std::function<void(size_t)> loadHexData);
void upload_block(GLuint tex, const std::vector<uint8_t> &raw, size_t block_width, size_t block_height);

} // namespace entropy