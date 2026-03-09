#pragma once

#include <entropy/app.h>
#include <string>

namespace entropy {

void startUpdateCheck(UiState &uiState, const std::string &file_path, bool manual = false);

} // namespace entropy
