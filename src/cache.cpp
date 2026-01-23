#include <algorithm>
#include <entropy/cache.h>
#include <entropy/core.h>
#include <fstream>
#include <thread>

namespace entropy {

std::atomic<size_t> cacheProgress = 0;
size_t cacheTotal = 0;
std::atomic<bool> cacheDone = true;
std::atomic<bool> cacheFailed = false;

bool load_block_from_file(std::ifstream &file, size_t block_index,
                          size_t block_size, size_t file_size,
                          std::vector<uint8_t> &buffer) {
    size_t start = block_index * block_size;
    if (start >= file_size)
        return false;

    file.clear(); // clear any EOF flags
    file.seekg(start, std::ios::beg);
    if (!file)
        return false;

    size_t remaining = file_size - start;
    size_t to_read = std::min(block_size, remaining);

    buffer.resize(to_read);
    file.read(reinterpret_cast<char *>(buffer.data()), to_read);

    return file.good() || file.eof();
}

void generateCacheThreaded(const std::string &input_file,
                           const std::string &cache_file) {
    const size_t SECTOR_SIZE = 512;

    std::string filename_only = fs::path(input_file).filename().string();

    std::ifstream file(input_file, std::ios::binary);
    if (!file) {
        cacheFailed = true;
        cacheDone = true;
        return;
    }

    size_t file_size = fs::file_size(input_file);
    cacheTotal = (file_size + SECTOR_SIZE - 1) / SECTOR_SIZE;

    std::ofstream cache(cache_file, std::ios::binary | std::ios::trunc);
    if (!cache) {
        cacheFailed = true;
        cacheDone = true;
        return;
    }

    std::vector<unsigned char> buffer(SECTOR_SIZE);

    for (size_t i = 0; i < cacheTotal; ++i) {
        file.seekg(i * SECTOR_SIZE);
        file.read(reinterpret_cast<char *>(buffer.data()), SECTOR_SIZE);

        size_t bytes_read = file.gcount();
        if (bytes_read == 0)
            break;

        double entropy = shannon_entropy(buffer.data(), bytes_read);
        double scaled = std::min(entropy, 7.99);
        uint8_t packed = pack_value(scaled);

        cache.write(reinterpret_cast<char *>(&packed), 1);

        if (i % 10000 == 0) {
            cacheProgress = i;
        }
    }

    cacheDone = true;
}

} // namespace entropy