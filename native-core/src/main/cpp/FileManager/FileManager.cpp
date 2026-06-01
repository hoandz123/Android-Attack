#include "FileManager.hpp"

#include <filesystem>
#include <fstream>

namespace fs {
namespace {

Result Ok() { return {}; }

Result Fail(const std::string &msg) {
    Result r;
    r.error = msg;
    return r;
}

Result FromEc(const std::error_code &ec) {
    if (!ec) return Ok();
    return Fail(ec.message());
}

std::filesystem::path p(const std::string &path) { return std::filesystem::path(path); }

} // namespace

bool Exists(const std::string &path) { return std::filesystem::exists(p(path)); }

bool IsFile(const std::string &path) {
    const std::filesystem::path fp = p(path);
    return std::filesystem::exists(fp) && std::filesystem::is_regular_file(fp);
}

bool IsDir(const std::string &path) {
    const std::filesystem::path fp = p(path);
    return std::filesystem::exists(fp) && std::filesystem::is_directory(fp);
}

int64_t FileSize(const std::string &path) {
    std::error_code ec;
    const auto n = std::filesystem::file_size(p(path), ec);
    return ec ? -1 : static_cast<int64_t>(n);
}

Result Remove(const std::string &path) {
    std::error_code ec;
    const std::filesystem::path fp = p(path);
    if (!std::filesystem::exists(fp, ec)) return FromEc(ec);
    if (std::filesystem::is_directory(fp, ec))
        std::filesystem::remove_all(fp, ec);
    else
        std::filesystem::remove(fp, ec);
    return FromEc(ec);
}

Result RenamePath(const std::string &from, const std::string &to) {
    std::error_code ec;
    std::filesystem::rename(p(from), p(to), ec);
    return FromEc(ec);
}

Result CopyFile(const std::string &src, const std::string &dst) {
    std::error_code ec;
    std::filesystem::copy_file(p(src), p(dst), std::filesystem::copy_options::overwrite_existing, ec);
    return FromEc(ec);
}

Result Mkdir(const std::string &path) {
    std::error_code ec;
    std::filesystem::create_directory(p(path), ec);
    return FromEc(ec);
}

Result MkdirP(const std::string &path) {
    std::error_code ec;
    std::filesystem::create_directories(p(path), ec);
    return FromEc(ec);
}

std::vector<std::string> ListDir(const std::string &dir) {
    std::vector<std::string> out;
    std::error_code ec;
    for (const auto &e : std::filesystem::directory_iterator(p(dir), ec)) {
        if (ec) break;
        out.push_back(e.path().filename().string());
    }
    return out;
}

Result WriteBytes(const std::string &path, const void *data, size_t len) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return Fail("fopen write");
    if (data && len) f.write(static_cast<const char *>(data), static_cast<std::streamsize>(len));
    return f.good() ? Ok() : Fail("write failed");
}

Result AppendBytes(const std::string &path, const void *data, size_t len) {
    std::ofstream f(path, std::ios::binary | std::ios::app);
    if (!f) return Fail("fopen append");
    if (data && len) f.write(static_cast<const char *>(data), static_cast<std::streamsize>(len));
    return f.good() ? Ok() : Fail("append failed");
}

std::vector<uint8_t> ReadBytes(const std::string &path, Result *out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        if (out) *out = Fail("fopen read");
        return {};
    }
    const auto n = f.tellg();
    if (n < 0) {
        if (out) *out = Fail("tellg");
        return {};
    }
    std::vector<uint8_t> buf(static_cast<size_t>(n));
    f.seekg(0);
    f.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(n));
    if (!f) {
        if (out) *out = Fail("read failed");
        return {};
    }
    if (out) *out = Ok();
    return buf;
}

std::string Join(const std::string &a, const std::string &b) { return (p(a) / b).string(); }

std::string Dirname(const std::string &path) { return p(path).parent_path().string(); }

std::string Basename(const std::string &path) { return p(path).filename().string(); }

} // namespace fs
