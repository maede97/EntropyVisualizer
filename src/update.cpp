#include <entropy/update.h>
#include <entropy/version.h>

#include <chrono>
#include <iostream>
#include <regex>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <windows.h>
#include <winhttp.h>
#endif

#if __linux__
#include <curl/curl.h>
#endif

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

        std::string response;
#ifdef __linux__
        CURL *curl = curl_easy_init();
        if (!curl) {
            uiState.updateChecked = true;
            return;
        }

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
#endif
#ifdef _WIN32
        DWORD dwSize = 0;
        DWORD dwDownloaded = 0;
        BOOL bResults = FALSE;

        HINTERNET hSession = NULL;
        HINTERNET hConnect = NULL;
        HINTERNET hRequest = NULL;

        URL_COMPONENTS urlComp{};
        urlComp.dwStructSize = sizeof(urlComp);
        urlComp.dwHostNameLength = (DWORD)-1;
        urlComp.dwUrlPathLength = (DWORD)-1;

        std::wstring wurl(full_url.begin(), full_url.end());

        if (!WinHttpCrackUrl(wurl.c_str(), (DWORD)wurl.size(), 0, &urlComp)) {
            uiState.updateChecked = true;
            return;
        }

        // Open session
        hSession = WinHttpOpen(L"EntropyVisualizer Update Checker/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY, WINHTTP_NO_PROXY_NAME,
                               WINHTTP_NO_PROXY_BYPASS, 0);

        // Connect
        if (hSession) {
            std::wstring host(urlComp.lpszHostName, urlComp.dwHostNameLength);

            hConnect = WinHttpConnect(hSession, host.c_str(), urlComp.nPort, 0);
        }

        // Open request
        if (hConnect) {
            std::wstring path(urlComp.lpszUrlPath, urlComp.dwUrlPathLength);

            hRequest = WinHttpOpenRequest(hConnect, L"GET", path.c_str(), NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                          (urlComp.nScheme == INTERNET_SCHEME_HTTPS) ? WINHTTP_FLAG_SECURE : 0);
        }

        // Send request
        if (hRequest) {
            bResults = WinHttpSendRequest(hRequest, WINHTTP_NO_ADDITIONAL_HEADERS, 0, WINHTTP_NO_REQUEST_DATA, 0, 0, 0);
        }

        // Receive response
        if (bResults)
            bResults = WinHttpReceiveResponse(hRequest, NULL);

        // Read response
        if (bResults) {
            do {
                dwSize = 0;

                if (!WinHttpQueryDataAvailable(hRequest, &dwSize))
                    break;

                if (!dwSize)
                    break;

                std::vector<char> buffer(dwSize + 1);
                ZeroMemory(buffer.data(), dwSize + 1);

                if (!WinHttpReadData(hRequest, buffer.data(), dwSize, &dwDownloaded)) {
                    break;
                }

                response.append(buffer.data(), dwDownloaded);

            } while (dwSize > 0);
        }

        // Cleanup
        if (hRequest)
            WinHttpCloseHandle(hRequest);
        if (hConnect)
            WinHttpCloseHandle(hConnect);
        if (hSession)
            WinHttpCloseHandle(hSession);

        uiState.updateChecked = true;
#endif

        // Trim whitespace
        std::string trimmed = response;
        while (!trimmed.empty() && isspace((unsigned char)trimmed.back()))
            trimmed.pop_back();
        size_t start = 0;
        while (start < trimmed.size() && isspace((unsigned char)trimmed[start]))
            start++;
        trimmed = (start < trimmed.size()) ? trimmed.substr(start) : std::string();

        std::string local_version = EV_VERSION;
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
