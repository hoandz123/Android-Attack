#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fs {

struct Result {
    std::string error;
    bool ok() const { return error.empty(); }
};

bool exists(const std::string &path);
bool is_file(const std::string &path);
bool is_dir(const std::string &path);
int64_t file_size(const std::string &path);

Result remove(const std::string &path);
Result rename_path(const std::string &from, const std::string &to);
Result copy_file(const std::string &src, const std::string &dst);

Result mkdir(const std::string &path);
Result mkdir_p(const std::string &path);

std::vector<std::string> list_dir(const std::string &dir);

Result write_bytes(const std::string &path, const void *data, size_t len);
Result append_bytes(const std::string &path, const void *data, size_t len);
std::vector<uint8_t> read_bytes(const std::string &path, Result *out = nullptr);

std::string join(const std::string &a, const std::string &b);
std::string dirname(const std::string &path);
std::string basename(const std::string &path);

} // namespace fs
