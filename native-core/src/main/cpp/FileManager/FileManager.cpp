#include "FileManager.hpp"

#include <filesystem>
#include <fstream>

namespace fs {
namespace {

Result ok() { return {}; }

Result fail(const std::string &msg) {
    Result r;
    r.error = msg;
    return r;
}

Result from_ec(const std::error_code &ec) {
    if (!ec) return ok();
    return fail(ec.message());
}

std::filesystem::path p(const std::string &path) { return std::filesystem::path(path); }

} // namespace

bool exists(const std::string &path) { return std::filesystem::exists(p(path)); }

bool is_file(const std::string &path) {
    const std::filesystem::path fp = p(path);
    return std::filesystem::exists(fp) && std::filesystem::is_regular_file(fp);
}

bool is_dir(const std::string &path) {
    const std::filesystem::path fp = p(path);
    return std::filesystem::exists(fp) && std::filesystem::is_directory(fp);
}

int64_t file_size(const std::string &path) {
    std::error_code ec;
    const auto n = std::filesystem::file_size(p(path), ec);
    return ec ? -1 : static_cast<int64_t>(n);
}

Result remove(const std::string &path) {
    std::error_code ec;
    const std::filesystem::path fp = p(path);
    if (!std::filesystem::exists(fp, ec)) return from_ec(ec);
    if (std::filesystem::is_directory(fp, ec))
        std::filesystem::remove_all(fp, ec);
    else
        std::filesystem::remove(fp, ec);
    return from_ec(ec);
}

Result rename_path(const std::string &from, const std::string &to) {
    std::error_code ec;
    std::filesystem::rename(p(from), p(to), ec);
    return from_ec(ec);
}

Result copy_file(const std::string &src, const std::string &dst) {
    std::error_code ec;
    std::filesystem::copy_file(p(src), p(dst), std::filesystem::copy_options::overwrite_existing, ec);
    return from_ec(ec);
}

Result mkdir(const std::string &path) {
    std::error_code ec;
    std::filesystem::create_directory(p(path), ec);
    return from_ec(ec);
}

Result mkdir_p(const std::string &path) {
    std::error_code ec;
    std::filesystem::create_directories(p(path), ec);
    return from_ec(ec);
}

std::vector<std::string> list_dir(const std::string &dir) {
    std::vector<std::string> out;
    std::error_code ec;
    for (const auto &e : std::filesystem::directory_iterator(p(dir), ec)) {
        if (ec) break;
        out.push_back(e.path().filename().string());
    }
    return out;
}

Result write_bytes(const std::string &path, const void *data, size_t len) {
    std::ofstream f(path, std::ios::binary | std::ios::trunc);
    if (!f) return fail("fopen write");
    if (data && len) f.write(static_cast<const char *>(data), static_cast<std::streamsize>(len));
    return f.good() ? ok() : fail("write failed");
}

Result append_bytes(const std::string &path, const void *data, size_t len) {
    std::ofstream f(path, std::ios::binary | std::ios::app);
    if (!f) return fail("fopen append");
    if (data && len) f.write(static_cast<const char *>(data), static_cast<std::streamsize>(len));
    return f.good() ? ok() : fail("append failed");
}

std::vector<uint8_t> read_bytes(const std::string &path, Result *out) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        if (out) *out = fail("fopen read");
        return {};
    }
    const auto n = f.tellg();
    if (n < 0) {
        if (out) *out = fail("tellg");
        return {};
    }
    std::vector<uint8_t> buf(static_cast<size_t>(n));
    f.seekg(0);
    f.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(n));
    if (!f) {
        if (out) *out = fail("read failed");
        return {};
    }
    if (out) *out = ok();
    return buf;
}

std::string join(const std::string &a, const std::string &b) { return (p(a) / b).string(); }

std::string dirname(const std::string &path) { return p(path).parent_path().string(); }

std::string basename(const std::string &path) { return p(path).filename().string(); }

} // namespace fs
