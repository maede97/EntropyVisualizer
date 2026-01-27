#include <entropy/app.h>
#include <entropy/cache.h>
#include <entropy/core.h>
#include <entropy/ui.h>

#include <GL/gl.h>
#include <algorithm>
#include <map>

namespace entropy {

// Upload one block into the active OpenGL texture
void upload_block(GLuint tex, const std::vector<uint8_t> &raw, size_t block_width, size_t block_height) {
    std::vector<uint8_t> tex_data(block_width * block_height * 4, 0);

    for (size_t i = 0; i < raw.size(); i++) {
        float e = unpack_value(raw[i]) / 8.0f;
        uint8_t r, g, b;
        value_to_color(e, r, g, b);

        tex_data[i * 4 + 0] = r;
        tex_data[i * 4 + 1] = g;
        tex_data[i * 4 + 2] = b;
        tex_data[i * 4 + 3] = 255;
    }

    glBindTexture(GL_TEXTURE_2D, tex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, block_width, block_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, tex_data.data());
}

void renderAboutWindow(UiState &uiState) {
    if (uiState.showAboutUs) {
        ImGui::SetNextWindowSize(ImVec2(500, 300), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("About", &uiState.showAboutUs)) {
            ImGui::Text(ABOUT_STRING.c_str(), "");
        }
        ImGui::End();
    }
}

void renderHelpWindow(UiState &uiState) {
    if (uiState.showHelp) {
        ImGui::SetNextWindowSize(ImVec2(600, 600), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Help", &uiState.showHelp)) {
            ImGui::Text(HELP_STRING.c_str(), "");
        }
        ImGui::End();
    }
}

void renderHexViewWindow(UiState &uiState, const AppState &appState) {
    if (uiState.showHexView) {
        ImGui::SetNextWindowSize(ImVec2(850, 650), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Hex View", &uiState.showHexView)) {
            ImGui::Text("Sector %zu (0x%zX)", uiState.currentSectorIndex, uiState.currentSectorIndex * 512);
            ImGui::Separator();

            if (uiState.currentSectorData.empty()) {
                ImGui::Text("No data available");
            } else {
                // Collect highlights from plugins
                std::map<size_t, std::pair<uint32_t, int>> highlightMap; // color, priority
                if (appState.hexDisplayFeatureManager) {
                    for (const auto *feature : appState.hexDisplayFeatureManager->getFeatures()) {
                        if (appState.featureEnabled.count(feature->getName()) && appState.featureEnabled.at(feature->getName())) {
                            auto highlights = feature->getHighlights(uiState.currentSectorData, uiState.currentSectorIndex);
                            int priority = feature->getPriority();
                            for (const auto &h : highlights) {
                                auto it = highlightMap.find(h.offset);
                                if (it == highlightMap.end() || it->second.second < priority) {
                                    highlightMap[h.offset] = {h.color, priority};
                                }
                            }
                        }
                    }
                }

                ImGui::BeginChild("HexContent", ImVec2(0, 0), true);
                for (size_t i = 0; i < uiState.currentSectorData.size(); i += 16) {
                    ImGui::Text("%08zX: ", i);
                    ImGui::SameLine();
                    for (size_t j = 0; j < 16 && i + j < uiState.currentSectorData.size(); ++j) {
                        size_t offset = i + j;
                        if (highlightMap.count(offset)) {
                            ImGui::TextColored(ImColor(highlightMap[offset].first), "%02X ", uiState.currentSectorData[offset]);
                        } else {
                            ImGui::Text("%02X ", uiState.currentSectorData[offset]);
                        }
                        ImGui::SameLine();
                    }
                    ImGui::Text(" | ");
                    ImGui::SameLine();
                    for (size_t j = 0; j < 16 && i + j < uiState.currentSectorData.size(); ++j) {
                        char c = uiState.currentSectorData[i + j];
                        size_t offset = i + j;
                        if (highlightMap.count(offset)) {
                            ImGui::TextColored(ImColor(highlightMap[offset].first), "%c", (c >= 32 && c <= 126) ? c : '.');
                        } else {
                            ImGui::Text("%c", (c >= 32 && c <= 126) ? c : '.');
                        }
                        if (j < 15)
                            ImGui::SameLine();
                    }
                }
                ImGui::EndChild();
            }
        }
        ImGui::End();
    }
}

void renderSearchWindow(UiState &uiState, AppState &appState, std::function<void(size_t)> loadHexData) {
    if (uiState.showSearchWindow) {
        ImGui::SetNextWindowSize(ImVec2(400, 250), ImGuiCond_FirstUseEver);
        if (ImGui::Begin("Search Sectors", &uiState.showSearchWindow)) {
            const char *items[] = {"Max Entropy (at most)", "Min Entropy (at least)", "Entropy Range"};
            ImGui::Combo("Search Type", &uiState.search_type, items, IM_ARRAYSIZE(items));

            if (uiState.search_type == 0) {
                ImGui::InputFloat("Max Entropy", &uiState.max_value);
            } else if (uiState.search_type == 1) {
                ImGui::InputFloat("Min Entropy", &uiState.min_value);
            } else {
                ImGui::InputFloat("Min Entropy", &uiState.range_min);
                ImGui::InputFloat("Max Entropy", &uiState.range_max);
            }

            if (ImGui::Button("Find")) {
                size_t found_index = SIZE_MAX;
                size_t start = (uiState.highlighted_sector == SIZE_MAX ? 0 : uiState.highlighted_sector + 1);
                if (uiState.search_type == 0) {
                    // max entropy (at most)
                    for (size_t i = start; i < appState.all_cache_data.size(); ++i) {
                        double e = unpack_value(appState.all_cache_data[i]);
                        if (e <= uiState.max_value) {
                            found_index = i;
                            break;
                        }
                    }
                    if (found_index == SIZE_MAX && start > 0) {
                        for (size_t i = 0; i < start; ++i) {
                            double e = unpack_value(appState.all_cache_data[i]);
                            if (e <= uiState.max_value) {
                                found_index = i;
                                break;
                            }
                        }
                    }
                } else if (uiState.search_type == 1) {
                    // min entropy (at least)
                    for (size_t i = start; i < appState.all_cache_data.size(); ++i) {
                        double e = unpack_value(appState.all_cache_data[i]);
                        if (e >= uiState.min_value) {
                            found_index = i;
                            break;
                        }
                    }
                    if (found_index == SIZE_MAX && start > 0) {
                        for (size_t i = 0; i < start; ++i) {
                            double e = unpack_value(appState.all_cache_data[i]);
                            if (e >= uiState.min_value) {
                                found_index = i;
                                break;
                            }
                        }
                    }
                } else {
                    // range
                    for (size_t i = start; i < appState.all_cache_data.size(); ++i) {
                        double e = unpack_value(appState.all_cache_data[i]);
                        if (e >= uiState.range_min && e <= uiState.range_max) {
                            found_index = i;
                            break;
                        }
                    }
                    if (found_index == SIZE_MAX && start > 0) {
                        for (size_t i = 0; i < start; ++i) {
                            double e = unpack_value(appState.all_cache_data[i]);
                            if (e >= uiState.range_min && e <= uiState.range_max) {
                                found_index = i;
                                break;
                            }
                        }
                    }
                }

                if (found_index != SIZE_MAX) {
                    uiState.highlighted_sector = found_index;
                    size_t block_size = appState.block_width * appState.block_height;
                    appState.current_block = found_index / block_size;
                    appState.block_slider = appState.current_block;
                    appState.redrawBlock = true;

                    loadHexData(found_index);
                }
            }
        }
        ImGui::End();
    }
}

void renderVisualization(ImDrawList *draw_list, GLuint tex, const std::vector<uint8_t> &block_buffer, float zoom, ImVec2 pan_offset,
                         size_t current_block, size_t block_size, size_t block_width, size_t block_height, UiState &uiState,
                         std::function<void(size_t)> loadHexData) {
    ImVec2 img_size(block_width * zoom, block_height * zoom);
    ImVec2 img_tl = ImVec2(-pan_offset.x, -pan_offset.y);
    ImVec2 img_br = ImVec2(img_tl.x + img_size.x, img_tl.y + img_size.y);
    draw_list->AddImage((ImTextureID)(intptr_t)tex, img_tl, img_br);

    // Rulers
    // Horizontal ruler (top)
    for (size_t i = 0; i <= block_width; i += 32) {
        float x = img_tl.x + i * zoom;
        if (x >= img_tl.x && x <= img_br.x) {
            draw_list->AddLine(ImVec2(x, img_tl.y - 10), ImVec2(x, img_tl.y), IM_COL32(255, 255, 255, 128), 1.0f);
            draw_list->AddText(ImVec2(x - 10, img_tl.y - 25), IM_COL32(255, 255, 255, 255), std::to_string(i).c_str());
        }
    }
    // Vertical ruler (left)
    for (size_t j = 0; j <= block_height; j += 32) {
        float y = img_tl.y + j * zoom;
        if (y >= img_tl.y && y <= img_br.y) {
            draw_list->AddLine(ImVec2(img_tl.x - 10, y), ImVec2(img_tl.x, y), IM_COL32(255, 255, 255, 128), 1.0f);
            draw_list->AddText(ImVec2(img_tl.x - 50, y - 7), IM_COL32(255, 255, 255, 255), std::to_string(j).c_str());
        }
    }
    // Horizontal ruler (bottom)
    for (size_t i = 0; i <= block_width; i += 32) {
        float x = img_tl.x + i * zoom;
        if (x >= img_tl.x && x <= img_br.x) {
            draw_list->AddLine(ImVec2(x, img_br.y - 10), ImVec2(x, img_br.y), IM_COL32(255, 255, 255, 128), 1.0f);
            draw_list->AddText(ImVec2(x - 10, img_br.y + 5), IM_COL32(255, 255, 255, 255), std::to_string(i).c_str());
        }
    }
    // Vertical ruler (right)
    for (size_t j = 0; j <= block_height; j += 32) {
        float y = img_tl.y + j * zoom;
        if (y >= img_tl.y && y <= img_br.y) {
            draw_list->AddLine(ImVec2(img_br.x - 10, y), ImVec2(img_br.x, y), IM_COL32(255, 255, 255, 128), 1.0f);
            draw_list->AddText(ImVec2(img_br.x + 5, y - 7), IM_COL32(255, 255, 255, 255), std::to_string(j).c_str());
        }
    }

    ImVec2 mouse_pos = ImGui::GetIO().MousePos;
    if (mouse_pos.x >= img_tl.x && mouse_pos.x < img_br.x && mouse_pos.y >= img_tl.y && mouse_pos.y < img_br.y) {
        float local_x = (mouse_pos.x + pan_offset.x) / zoom;
        float local_y = (mouse_pos.y + pan_offset.y) / zoom;

        size_t pixel_x = (size_t)std::max(0.0f, std::min(local_x, (float)block_width - 1));
        size_t pixel_y = (size_t)std::max(0.0f, std::min(local_y, (float)block_height - 1));

        size_t idx = pixel_y * block_width + pixel_x;
        if (idx < block_buffer.size()) {
            float entropy = unpack_value(block_buffer[idx]);
            ImGui::SetTooltip("Sector %zu (%zu, %zu)\nEntropy: %.3f", current_block * block_size + idx, pixel_x, pixel_y, entropy);

            // Highlight hovered pixel
            ImVec2 highlight_tl = ImVec2(img_tl.x + pixel_x * zoom, img_tl.y + pixel_y * zoom);
            ImVec2 highlight_br = ImVec2(highlight_tl.x + zoom, highlight_tl.y + zoom);
            draw_list->AddRect(highlight_tl, highlight_br, IM_COL32(255, 255, 255, 255), 0.0f, 0, 2.0f);

            // Handle click
            if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse) {
                size_t sector_index = current_block * block_size + idx;
                uiState.highlighted_sector = sector_index;
                loadHexData(sector_index);
                uiState.showHexView = true;
            }

            if (ImGui::IsMouseDragging(ImGuiMouseButton_Left) && !ImGui::GetIO().WantCaptureMouse) {
                size_t sector_index = current_block * block_size + idx;
                uiState.highlighted_sector = sector_index;
                loadHexData(sector_index);
                uiState.showHexView = true;
            }
        }
    }

    // Persistent pink highlight for selected sector
    if (uiState.highlighted_sector != SIZE_MAX) {
        size_t sel_block = uiState.highlighted_sector / block_size;
        if (sel_block == current_block) {
            size_t local_idx = uiState.highlighted_sector % block_size;
            size_t sel_x = local_idx % block_width;
            size_t sel_y = local_idx / block_width;
            if (local_idx < block_buffer.size()) {
                ImVec2 sel_tl = ImVec2(img_tl.x + sel_x * zoom, img_tl.y + sel_y * zoom);
                ImVec2 sel_br = ImVec2(sel_tl.x + zoom, sel_tl.y + zoom);
                draw_list->AddRect(sel_tl, sel_br, IM_COL32(255, 0, 255, 255), 0.0f, 0, 3.0f);

                // Highlight in rulers
                float ruler_x = img_tl.x + sel_x * zoom;
                draw_list->AddLine(ImVec2(ruler_x, img_tl.y - 20), ImVec2(ruler_x, img_tl.y), IM_COL32(255, 0, 255, 255), 2.0f);
                float ruler_y = img_tl.y + sel_y * zoom;
                draw_list->AddLine(ImVec2(img_tl.x - 20, ruler_y), ImVec2(img_tl.x, ruler_y), IM_COL32(255, 0, 255, 255), 2.0f);
                // Lower right marking
                draw_list->AddLine(ImVec2(ruler_x + zoom, img_tl.y - 20), ImVec2(ruler_x + zoom, img_tl.y), IM_COL32(255, 0, 255, 255), 2.0f);
                draw_list->AddLine(ImVec2(img_tl.x, ruler_y + zoom), ImVec2(img_tl.x - 20, ruler_y + zoom), IM_COL32(255, 0, 255, 255), 2.0f);
            }
        }
    }
}

void handleFileDialogs(UiState &uiState, AppState &appState, IGFD::FileDialogConfig &config, std::function<void(size_t)> loadHexData) {
    ImVec2 maxSize = ImVec2(1800, 1200);               // The full display area
    ImVec2 minSize = ImVec2(1800 / 2.0f, 1200 / 2.0f); // Half the display area

    if (ImGuiFileDialog::Instance()->Display("OpenCacheDlg", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string filePathName = ImGuiFileDialog::Instance()->GetFilePathName();
            appState.lastCacheFile = filePathName;
            appState.originalFile = ""; // Reset original file
            appState.file.close();
            appState.file.open(filePathName, std::ios::binary);
            if (appState.file) {
                appState.file.seekg(0, std::ios::end);
                appState.file_size = (size_t)appState.file.tellg();
                appState.file.seekg(0, std::ios::beg);
                appState.all_cache_data.resize(appState.file_size);
                appState.file.read(reinterpret_cast<char *>(appState.all_cache_data.data()), appState.file_size);
                appState.current_block = 0;
                appState.block_slider = 0;
                appState.redrawBlock = true;
                appState.promptForSource = true; // Always prompt for source when opening cache
                // Clear old hex view data
                uiState.highlighted_sector = SIZE_MAX;
                uiState.currentSectorData.clear();
                uiState.currentSectorIndex = 0;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("GenerateFileDlg", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string fileToCache = ImGuiFileDialog::Instance()->GetFilePathName();
            appState.showCacheGen = true;
            cacheDone = false;
            cacheFailed = false;
            cacheProgress = 0;
            std::string filename_only = fs::path(fileToCache).filename().string();
            std::string cacheFile = (fs::temp_directory_path() / (filename_only + ".cache.bin")).string();
            appState.lastCacheFile = cacheFile;
            appState.originalFile = fileToCache;
            appState.cacheThread = std::thread([fileToCache, cacheFile]() { generateCacheThreaded(fileToCache, cacheFile); });
            appState.cacheThread.detach();
        }
        ImGuiFileDialog::Instance()->Close();
    }

    if (ImGuiFileDialog::Instance()->Display("SelectSourceDlg", ImGuiWindowFlags_NoCollapse, minSize, maxSize)) {
        if (ImGuiFileDialog::Instance()->IsOk()) {
            std::string sourcePath = ImGuiFileDialog::Instance()->GetFilePathName();
            // Check if source file size matches cache
            std::ifstream source(sourcePath, std::ios::binary);
            if (source) {
                source.seekg(0, std::ios::end);
                size_t source_size = (size_t)source.tellg();
                size_t expected_cache_size = (source_size + 511) / 512; // SECTOR_SIZE = 512
                if (expected_cache_size == appState.file_size) {
                    appState.originalFile = sourcePath;
                    appState.promptForSource = false;
                    // Reload hex view if open
                    if (uiState.showHexView) {
                        loadHexData(uiState.currentSectorIndex);
                    }
                } else {
                    // Size mismatch, do not set
                }
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
}

} // namespace entropy