#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace fs {

struct Result {
    std::string error;
    bool ok() const { return error.empty(); }
};

bool Exists(const std::string &path);
bool IsFile(const std::string &path);
bool IsDir(const std::string &path);
int64_t FileSize(const std::string &path);

Result Remove(const std::string &path);
Result RenamePath(const std::string &from, const std::string &to);
Result CopyFile(const std::string &src, const std::string &dst);

Result Mkdir(const std::string &path);
Result MkdirP(const std::string &path);

std::vector<std::string> ListDir(const std::string &dir);

Result WriteBytes(const std::string &path, const void *data, size_t len);
Result AppendBytes(const std::string &path, const void *data, size_t len);
std::vector<uint8_t> ReadBytes(const std::string &path, Result *out = nullptr);

std::string Join(const std::string &a, const std::string &b);
std::string Dirname(const std::string &path);
std::string Basename(const std::string &path);

} // namespace fs
