#define IMGUI_DEFINE_MATH_OPERATORS

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <algorithm>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_opengl3.h>
#include <cctype>
#include <cstring>
#include <glad/glad.h>
#include <imgui.h>
#include <imgui_internal.h>
#include <iostream>

#include <entropy/app.h>
#include <entropy/cache.h>
#include <entropy/core.h>
#include <entropy/icon.h>
#include <entropy/ui.h>
#include <entropy/update.h>
#include <entropy/version.h>

entropy::AppState *globalAppState = nullptr;

int main(int argc, char **argv) {
    entropy::AppState appState;
    entropy::UiState uiState;

    if (entropy::parseCommandLine(argc, argv, appState) != 0) {
        return 1;
    }

    // Initialize hex display feature manager
    appState.hexDisplayFeatureManager = std::make_unique<entropy::HexDisplayFeatureManager>();

    // Initialize feature enabled states
    for (const auto *feature : appState.hexDisplayFeatureManager->getFeatures()) {
        appState.featureEnabled[feature->getName()] = true;
    }

    globalAppState = &appState;

    GLFWwindow *window;
    GLuint tex;
    entropy::initializeWindowAndGL(window, tex);

    if (!window) {
        std::cerr << "Failed to create window\n";
        return 1;
    }

    // Start background update check (non-blocking)
    entropy::startUpdateCheck(uiState, "VERSION.txt");

    // Register ImGui settings handler for features
    ImGuiSettingsHandler featuresHandler;
    featuresHandler.TypeName = "HexDisplayFeatures";
    featuresHandler.TypeHash = ImHashStr("HexDisplayFeatures");
    featuresHandler.ReadOpenFn = [](ImGuiContext *, ImGuiSettingsHandler *, const char *name) -> void * {
        if (strcmp(name, "Enabled") == 0)
            // return string "Enabled" as a void* (which is later used to infer the section)
            return (void *)"Enabled";

        if (strcmp(name, "Gradient") == 0)
            return (void *)"Gradient";

        // check if name matches any feature slug
        for (const auto *f : globalAppState->hexDisplayFeatureManager->getFeatures()) {
            std::string slug = f->getSlug();
            if (slug == name) {
                return (void *)f;
            }
        }

        return nullptr;
    };
    featuresHandler.ReadLineFn = [](ImGuiContext *, ImGuiSettingsHandler *, void *entry, const char *line) {
        if (entry == (void *)"Enabled") {
            std::string line_str = line;
            size_t eq_pos = line_str.find('=');
            if (eq_pos != std::string::npos) {
                std::string slug = line_str.substr(0, eq_pos);
                std::string value = line_str.substr(eq_pos + 1);

                std::string featureName;
                // Find feature by slug
                for (const auto *f : globalAppState->hexDisplayFeatureManager->getFeatures()) {
                    std::string f_slug = f->getSlug();
                    if (f_slug == slug) {
                        featureName = f->getName();
                        break;
                    }
                }
                if (featureName.empty())
                    return; // Unknown feature

                globalAppState->featureEnabled[featureName] = (value == "1");
            }
        }

        if (entry == (void *)"Gradient") {
            std::string line_str = line;
            size_t eq_pos = line_str.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = line_str.substr(0, eq_pos);
                std::string value = line_str.substr(eq_pos + 1);

                if (key == "marks") {
                    // Parse marks
                    std::list<ImGG::Mark> marks;
                    size_t start = 0;
                    while (start < value.length()) {
                        size_t comma_pos = value.find(',', start);
                        std::string mark_str = (comma_pos == std::string::npos) ? value.substr(start) : value.substr(start, comma_pos - start);

                        size_t colon_pos = mark_str.find(':');
                        if (colon_pos != std::string::npos) {
                            float pos = std::stof(mark_str.substr(0, colon_pos));
                            std::string color_str = mark_str.substr(colon_pos + 1);
                            unsigned int color_value = 0;
                            if (sscanf(color_str.c_str(), "0x%X", &color_value) == 1) {
                                ImGG::Mark mark;
                                mark.position = ImGG::RelativePosition{pos};
                                mark.color = ImColor(color_value);
                                marks.push_back(mark);
                            }
                        }

                        if (comma_pos == std::string::npos)
                            break;
                        start = comma_pos + 1;
                    }

                    globalAppState->gradient_widget.gradient() = ImGG::Gradient{marks};
                }

                if (key == "interpolation") {
                    int mode = std::stoi(value);
                    globalAppState->gradient_widget.gradient().interpolation_mode() = static_cast<ImGG::Interpolation>(mode);
                }

                return;
            }
        }

        // Settings for each hex display feature
        auto *feature = static_cast<entropy::HexDisplayFeature *>(entry);
        std::string line_str = line;
        size_t eq_pos = line_str.find('=');
        if (eq_pos != std::string::npos) {
            std::string key = line_str.substr(0, eq_pos);
            std::string value = line_str.substr(eq_pos + 1);

            feature->setConfig(key, value);
        }
    };
    featuresHandler.WriteAllFn = [](ImGuiContext *, ImGuiSettingsHandler *handler, ImGuiTextBuffer *buf) {
        buf->appendf("[%s][Enabled]\n", handler->TypeName);
        for (const auto &pair : globalAppState->featureEnabled) {
            std::string name = pair.first;
            std::replace(name.begin(), name.end(), ' ', '_');
            buf->appendf("%s=%d\n", name.c_str(), pair.second ? 1 : 0);
        }
        buf->appendf("\n");

        // Gradient settings
        buf->appendf("[%s][Gradient]\n", handler->TypeName);
        const auto &gradient = globalAppState->gradient_widget.gradient();
        buf->appendf("marks=");
        bool first = true;
        for (const auto &mark : gradient.get_marks()) {
            if (!first) {
                buf->appendf(",");
            }
            first = false;
            unsigned int color_value = ImGui::ColorConvertFloat4ToU32(mark.color);
            buf->appendf("%.6f:0x%08X", mark.position.get(), color_value);
        }
        buf->appendf("\n");
        buf->appendf("interpolation=%d\n", static_cast<int>(gradient.interpolation_mode()));
        buf->appendf("\n");

        // config for each feature
        for (const auto *f : globalAppState->hexDisplayFeatureManager->getFeatures()) {
            std::string slug = f->getSlug();

            buf->appendf("[%s][%s]\n", handler->TypeName, slug.c_str());

            std::vector<std::string> configOptions = f->getConfigOptions();
            for (const auto &key : configOptions) {
                std::string value = f->getConfigValue(key);
                buf->appendf("%s=%s\n", key.c_str(), value.c_str());
            }

            buf->appendf("\n");
        }
    };
    // buf->appendf("color=0x%08X\n", f->color);

    ImGui::AddSettingsHandler(&featuresHandler);

    appState.resetHexDisplayGradientColors();

    // Load settings
    ImGui::LoadIniSettingsFromDisk(ImGui::GetIO().IniFilename);

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
    config.flags = ImGuiFileDialogFlags_ShowDevicesButton | ImGuiFileDialogFlags_Modal | ImGuiFileDialogFlags_DisableCreateDirectoryButton;

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
