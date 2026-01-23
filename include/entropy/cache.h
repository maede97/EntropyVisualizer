#pragma once

#include <atomic>
#include <filesystem>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace entropy {

const size_t SECTOR_SIZE = 512;

extern std::atomic<size_t> cacheProgress;
extern size_t cacheTotal;
extern std::atomic<bool> cacheDone;
extern std::atomic<bool> cacheFailed;

void generateCacheThreaded(const std::string &input_file, const std::string &cache_file);
bool load_block_from_file(std::ifstream &file, size_t block_index, size_t block_size, size_t file_size, std::vector<uint8_t> &buffer);

} // namespace entropy