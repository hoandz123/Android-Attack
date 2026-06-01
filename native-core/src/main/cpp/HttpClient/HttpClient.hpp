#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace http {

struct Response {
    long status = 0;
    std::vector<uint8_t> body;
    std::string error;
    bool ok() const { return error.empty() && status >= 200 && status < 300; }
};

Response get(const std::string &url, long timeout_sec = 30);
Response post(const std::string &url, const void *body = nullptr, size_t body_len = 0,
              const char *content_type = "application/octet-stream", long timeout_sec = 30);
/** GET → ghi file `path` (không giữ body trong Response). */
Response download(const std::string &url, const std::string &path, long timeout_sec = 30);

} // namespace http
