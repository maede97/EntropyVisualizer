#include <entropy/update.h>
#include <entropy/version.h>

#include <chrono>
#include <curl/curl.h>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>

namespace entropy {

static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t total = size * nmemb;
    std::string *s = static_cast<std::string *>(userp);
    s->append(static_cast<char *>(contents), total);
    return total;
}

static bool parse_semver(const std::string &s, int &major, int &minor, int &patch) {
    std::regex re(R"((\d+)\.(\d+)\.(\d+))");
    std::smatch m;
    if (std::regex_search(s, m, re)) {
        major = std::stoi(m[1]);
        minor = std::stoi(m[2]);
        patch = std::stoi(m[3]);
        return true;
    }
    return false;
}

static int compare_semver(const std::string &a, const std::string &b) {
    int am = 0, an = 0, ap = 0;
    int bm = 0, bn = 0, bp = 0;
    if (!parse_semver(a, am, an, ap) || !parse_semver(b, bm, bn, bp)) {
        return 0; // unknown; treat as equal
    }
    if (am != bm)
        return (am < bm) ? -1 : 1;
    if (an != bn)
        return (an < bn) ? -1 : 1;
    if (ap != bp)
        return (ap < bp) ? -1 : 1;
    return 0;
}

void startUpdateCheck(UiState &uiState, const std::string &file_path, bool manual) {
    // Mark manual request state
    if (manual)
        uiState.updateManualRequest = true;

    // Run async in a detached thread
    std::thread([&uiState, file_path, manual]() {
        const std::string base = "https://raw.githubusercontent.com/maede97/EntropyVisualizer/refs/heads/main/";
        const std::string full_url = base + file_path;
        const std::string release_page = "https://github.com/maede97/EntropyVisualizer/releases";

        CURL *curl = curl_easy_init();
        if (!curl) {
            uiState.updateChecked = true;
            return;
        }

        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, full_url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        CURLcode res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            curl_easy_cleanup(curl);
            // Always mark checked; for manual requests we want the UI to show a result
            uiState.updateAvailable = false;
            uiState.updateChecked = true;
            return;
        }

        curl_easy_cleanup(curl);

        // Trim whitespace
        std::string trimmed = response;

        std::cout << "Latest version string from server: '" << trimmed << "'" << std::endl;

        while (!trimmed.empty() && isspace((unsigned char)trimmed.back()))
            trimmed.pop_back();
        size_t start = 0;
        while (start < trimmed.size() && isspace((unsigned char)trimmed[start]))
            start++;
        trimmed = (start < trimmed.size()) ? trimmed.substr(start) : std::string();

        std::string local_version = VERSION;
        std::string remote_version = trimmed;

        int cmp = compare_semver(local_version, remote_version);
        if (cmp < 0) {
            // remote is newer
            {
                std::lock_guard<std::mutex> lk(uiState.updateMutex);
                uiState.latestVersion = remote_version;
                uiState.updateUrl = release_page;
            }
            uiState.updateAvailable = true;
        } else {
            uiState.updateAvailable = false;
        }
        // Mark checked so UI can react; manual requests will show the window even when not available
        uiState.updateChecked = true;
    }).detach();
}

} // namespace entropy
