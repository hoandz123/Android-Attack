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
    bool cache_hit() const { return error.empty() && status == 304; }
};

struct CacheValidators {
    std::string etag;
    std::string last_modified;
};

Response Get(const std::string &url, long timeout_sec = 30);
Response Post(const std::string &url, const void *body = nullptr, size_t body_len = 0,
              const char *content_type = "application/octet-stream", long timeout_sec = 30);
/** GET → ghi file `path` (không giữ body trong Response). */
Response Download(const std::string &url, const std::string &path, long timeout_sec = 30);

/** GET → `path` qua file tạm `path.tmp`; `in` gửi If-None-Match / If-Modified-Since. 304 giữ `path`. */
Response DownloadConditional(const std::string &url, const std::string &path, long timeout_sec,
                             const CacheValidators *in, CacheValidators *out);

} // namespace http
