#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <glad/glad.h>
#include <imgui.h>
#include <iostream>

#include "app.h"
#include "cache.h"
#include "core.h"
#include "icon.h"
#include "ui.h"
#include "version.h"

int main(int argc, char **argv) {
    entropy::AppState appState;
    entropy::UiState uiState;

    if (entropy::parseCommandLine(argc, argv, appState) != 0) {
        return 1;
    }

    GLFWwindow *window;
    GLuint tex;
    entropy::initializeWindowAndGL(window, tex);

    if (!window) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    // Set up drop callback
    glfwSetWindowUserPointer(window, &appState.droppedFiles);
    glfwSetDropCallback(window, [](GLFWwindow *window, int count, const char **paths) {
        auto *dropped = static_cast<std::vector<std::string> *>(glfwGetWindowUserPointer(window));
        if (!dropped)
            return;
        for (int i = 0; i < count; ++i) {
            dropped->emplace_back(paths[i]);
        }
    });

    IGFD::FileDialogConfig config;
    config.flags = ImGuiFileDialogFlags_ShowDevicesButton | ImGuiFileDialogFlags_Modal |
                   ImGuiFileDialogFlags_DisableCreateDirectoryButton;

    auto loadHexData = [&](size_t sector_index) {
        uiState.currentSectorIndex = sector_index;
        if (appState.originalFile.length() > 0) {
            uiState.currentSectorData.resize(512);
            std::ifstream orig_file(appState.originalFile, std::ios::binary);
            if (orig_file) {
                orig_file.seekg(sector_index * 512);
                orig_file.read(reinterpret_cast<char *>(uiState.currentSectorData.data()), 512);
                size_t bytes_read = orig_file.gcount();
                uiState.currentSectorData.resize(bytes_read);
            } else {
                uiState.currentSectorData.clear();
            }
        } else {
            uiState.currentSectorData.clear();
        }
    };

    entropy::mainLoop(window, tex, appState, uiState, config, loadHexData);

    // Cleanup
    glDeleteTextures(1, &tex);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
