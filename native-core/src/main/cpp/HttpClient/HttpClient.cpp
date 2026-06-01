#include "HttpClient.hpp"
#include <cstdio>
#include <curl/curl.h>
#include <mutex>

namespace http {
namespace {

std::once_flag g_once;

size_t VecWrite(char *p, size_t sz, size_t n, void *out) {
    auto *v = static_cast<std::vector<uint8_t> *>(out);
    v->insert(v->end(), reinterpret_cast<uint8_t *>(p), reinterpret_cast<uint8_t *>(p) + sz * n);
    return sz * n;
}

size_t FileWrite(char *p, size_t sz, size_t n, void *out) {
    return fwrite(p, 1, sz * n, static_cast<FILE *>(out));
}

void SetupCommon(CURL *c, const std::string &url, long timeout) {
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);
}

Response Finish(CURL *c, CURLcode e, Response r) {
    if (e != CURLE_OK)
        r.error = curl_easy_strerror(e);
    else
        curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &r.status);
    curl_easy_cleanup(c);
    return r;
}

Response Request(bool post, const std::string &url, const void *body, size_t body_len,
                 const char *ctype, long timeout) {
    std::call_once(g_once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
    Response r;
    CURL *c = curl_easy_init();
    if (!c) {
        r.error = "curl init";
        return r;
    }

    std::vector<uint8_t> buf;
    SetupCommon(c, url, timeout);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, VecWrite);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, &buf);

    curl_slist *hdrs = nullptr;
    if (post) {
        curl_easy_setopt(c, CURLOPT_POST, 1L);
        curl_easy_setopt(c, CURLOPT_POSTFIELDS, body && body_len ? body : "");
        curl_easy_setopt(c, CURLOPT_POSTFIELDSIZE, static_cast<long>(body_len));
        if (ctype) {
            std::string h = std::string("Content-Type: ") + ctype;
            hdrs = curl_slist_append(hdrs, h.c_str());
            curl_easy_setopt(c, CURLOPT_HTTPHEADER, hdrs);
        }
    }
    r = Finish(c, curl_easy_perform(c), r);
    curl_slist_free_all(hdrs);
    if (r.ok()) r.body = std::move(buf);
    return r;
}

} // namespace

Response Get(const std::string &url, long t) { return Request(false, url, nullptr, 0, nullptr, t); }

Response Post(const std::string &url, const void *body, size_t n, const char *ctype, long t) {
    return Request(true, url, body, n, ctype, t);
}

Response Download(const std::string &url, const std::string &path, long t) {
    std::call_once(g_once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
    Response r;
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) {
        r.error = "fopen failed";
        return r;
    }
    CURL *c = curl_easy_init();
    if (!c) {
        std::fclose(f);
        r.error = "curl init";
        return r;
    }
    SetupCommon(c, url, t);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, FileWrite);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, f);
    const CURLcode res = curl_easy_perform(c);
    long http_code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &http_code);
    curl_easy_cleanup(c);
    std::fflush(f);
    const long bytes = std::ftell(f);
    std::fclose(f);
    if (res != CURLE_OK) {
        r.error = curl_easy_strerror(res);
        std::remove(path.c_str());
        return r;
    }
    r.status = http_code;
    if (http_code < 200 || http_code >= 300) {
        r.error = "http " + std::to_string(http_code);
        std::remove(path.c_str());
        return r;
    }
    if (bytes <= 0) {
        r.error = "empty file";
        std::remove(path.c_str());
        return r;
    }
    return r;
}

} // namespace http
