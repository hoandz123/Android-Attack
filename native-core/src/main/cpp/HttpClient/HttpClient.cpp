#include "HttpClient.hpp"
#include <FileManager.hpp>
#include <android/log.h>
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <curl/curl.h>
#include <mutex>

namespace http {
namespace {

constexpr const char *kTag = "AttackHttp";

#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, kTag, __VA_ARGS__)
#define LOGW(...) __android_log_print(ANDROID_LOG_WARN, kTag, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, kTag, __VA_ARGS__)

std::once_flag g_once;

size_t VecWrite(char *p, size_t sz, size_t n, void *out) {
    auto *v = static_cast<std::vector<uint8_t> *>(out);
    v->insert(v->end(), reinterpret_cast<uint8_t *>(p), reinterpret_cast<uint8_t *>(p) + sz * n);
    return sz * n;
}

int CurlDebug(CURL *, curl_infotype type, char *data, size_t size, void *) {
    if (!data || size == 0) return 0;
    if (type == CURLINFO_DATA_IN || type == CURLINFO_DATA_OUT) {
        LOGI("curl[data]: %zu bytes", size);
        return 0;
    }
    while (size > 0 && (data[size - 1] == '\n' || data[size - 1] == '\r')) --size;
    const char *kind = "info";
    if (type == CURLINFO_TEXT) kind = "text";
    else if (type == CURLINFO_HEADER_IN) kind = "hdr<-";
    else if (type == CURLINFO_HEADER_OUT) kind = "hdr->";
    else if (type == CURLINFO_SSL_DATA_IN) kind = "ssl<-";
    else if (type == CURLINFO_SSL_DATA_OUT) kind = "ssl->";
    LOGI("curl[%s]: %.*s", kind, (int) size, data);
    return 0;
}

void RemovePath(const std::string &path, const char *why) {
    LOGW("remove path=%s reason=%s", path.c_str(), why);
    const int rc = std::remove(path.c_str());
    if (rc != 0) LOGE("remove failed path=%s rc=%d errno=%d (%s)", path.c_str(), rc, errno, std::strerror(errno));
    else LOGI("remove ok path=%s", path.c_str());
}

void SetupCommon(CURL *c, const std::string &url, long timeout) {
    curl_easy_setopt(c, CURLOPT_URL, url.c_str());
    curl_easy_setopt(c, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(c, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(c, CURLOPT_TIMEOUT, timeout);
    curl_easy_setopt(c, CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(c, CURLOPT_SSL_VERIFYHOST, 0L);
    curl_easy_setopt(c, CURLOPT_HTTP_VERSION, CURL_HTTP_VERSION_1_1);
    curl_easy_setopt(c, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
    curl_easy_setopt(c, CURLOPT_USERAGENT, "Mozilla/5.0 (Linux; Android) AppMod/1.0");
}

void SetupVerbose(CURL *c, char *errbuf) {
    curl_easy_setopt(c, CURLOPT_ERRORBUFFER, errbuf);
    curl_easy_setopt(c, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(c, CURLOPT_DEBUGFUNCTION, CurlDebug);
    curl_easy_setopt(c, CURLOPT_DEBUGDATA, nullptr);
}

struct HeaderCapture {
    std::string *etag = nullptr;
    std::string *last_modified = nullptr;
};

void TrimInPlace(std::string &s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t')) s.erase(s.begin());
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r' || s.back() == '\n')) s.pop_back();
}

size_t CaptureHeader(char *buffer, size_t size, size_t nitems, void *userdata) {
    const size_t total = size * nitems;
    auto *cap = static_cast<HeaderCapture *>(userdata);
    if (!cap || !buffer || total == 0) return total;
    std::string line(buffer, total);
    TrimInPlace(line);
    if (line.empty()) return total;
    const auto colon = line.find(':');
    if (colon == std::string::npos) return total;
    std::string name = line.substr(0, colon);
    std::string value = line.substr(colon + 1);
    TrimInPlace(name);
    TrimInPlace(value);
    for (auto &c : name) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    if (name == "etag" && cap->etag && !value.empty()) *cap->etag = value;
    else if (name == "last-modified" && cap->last_modified && !value.empty()) *cap->last_modified = value;
    return total;
}

curl_slist *BuildConditionalHeaders(const CacheValidators *in) {
    curl_slist *hdrs = nullptr;
    if (!in) return hdrs;
    if (!in->etag.empty()) {
        const std::string h = "If-None-Match: " + in->etag;
        hdrs = curl_slist_append(hdrs, h.c_str());
    }
    if (!in->last_modified.empty()) {
        const std::string h = "If-Modified-Since: " + in->last_modified;
        hdrs = curl_slist_append(hdrs, h.c_str());
    }
    return hdrs;
}

Response Finish(CURL *c, CURLcode e, Response r) {
    if (e != CURLE_OK) r.error = curl_easy_strerror(e);
    else curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &r.status);
    curl_easy_cleanup(c);
    return r;
}

Response Request(bool post, const std::string &url, const void *body, size_t body_len, const char *ctype, long timeout) {
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
    LOGI("Download begin url=%s path=%s timeout=%lds", url.c_str(), path.c_str(), t);
    Response r;
    char errbuf[CURL_ERROR_SIZE]{};
    FILE *f = std::fopen(path.c_str(), "wb");
    if (!f) {
        r.error = "fopen failed";
        LOGE("Download fopen failed path=%s errno=%d (%s)", path.c_str(), errno, std::strerror(errno));
        return r;
    }
    LOGI("Download fopen ok path=%s (0-byte file created)", path.c_str());
    CURL *c = curl_easy_init();
    if (!c) {
        std::fclose(f);
        RemovePath(path, "curl init failed");
        r.error = "curl init";
        LOGE("Download curl_easy_init failed");
        return r;
    }
    SetupCommon(c, url, t);
    SetupVerbose(c, errbuf);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, f);
    const CURLcode res = curl_easy_perform(c);
    long http_code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &http_code);
    char *effective_url = nullptr;
    curl_easy_getinfo(c, CURLINFO_EFFECTIVE_URL, &effective_url);
    curl_off_t curl_bytes = 0;
    curl_easy_getinfo(c, CURLINFO_SIZE_DOWNLOAD_T, &curl_bytes);
    curl_easy_cleanup(c);
    std::fflush(f);
    const long ftell_bytes = std::ftell(f);
    std::fclose(f);
    LOGI("Download perform res=%d (%s) http=%ld curl_bytes=%lld ftell=%ld errbuf=%s effective=%s", (int) res, curl_easy_strerror(res), http_code, (long long) curl_bytes, ftell_bytes, errbuf[0] ? errbuf : "(empty)", effective_url ? effective_url : "?");
    if (res != CURLE_OK) {
        r.error = curl_easy_strerror(res);
        if (errbuf[0]) r.error += std::string(" | ") + errbuf;
        RemovePath(path, "curl perform failed");
        LOGE("Download KEPT=no decision=deleted curl_error");
        return r;
    }
    r.status = http_code;
    if (http_code < 200 || http_code >= 300) {
        r.error = "http " + std::to_string(http_code);
        RemovePath(path, "bad http status");
        LOGE("Download KEPT=no decision=deleted http=%ld", http_code);
        return r;
    }
    if (ftell_bytes <= 0 || curl_bytes <= 0) {
        r.error = "empty file";
        RemovePath(path, "zero bytes written");
        LOGE("Download KEPT=no decision=deleted empty body ftell=%ld curl_bytes=%lld", ftell_bytes, (long long) curl_bytes);
        return r;
    }
    LOGI("Download KEPT=yes path=%s bytes=%ld", path.c_str(), ftell_bytes);
    return r;
}

Response DownloadConditional(const std::string &url, const std::string &path, long t,
                             const CacheValidators *in, CacheValidators *out) {
    std::call_once(g_once, [] { curl_global_init(CURL_GLOBAL_DEFAULT); });
    const std::string tmp = path + ".tmp";
    LOGI("DownloadConditional begin url=%s path=%s tmp=%s timeout=%lds conditional=%d", url.c_str(), path.c_str(), tmp.c_str(), t, in ? 1 : 0);
    if (in) {
        LOGI("DownloadConditional request if-none-match=%s if-modified-since=%s", in->etag.empty() ? "(none)" : in->etag.c_str(), in->last_modified.empty() ? "(none)" : in->last_modified.c_str());
    }
    Response r;
    char errbuf[CURL_ERROR_SIZE]{};
    FILE *f = std::fopen(tmp.c_str(), "wb");
    if (!f) {
        r.error = "fopen tmp failed";
        LOGE("DownloadConditional fopen failed tmp=%s errno=%d (%s)", tmp.c_str(), errno, std::strerror(errno));
        return r;
    }
    CURL *c = curl_easy_init();
    if (!c) {
        std::fclose(f);
        RemovePath(tmp, "curl init failed");
        r.error = "curl init";
        return r;
    }
    SetupCommon(c, url, t);
    SetupVerbose(c, errbuf);
    curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, nullptr);
    curl_easy_setopt(c, CURLOPT_WRITEDATA, f);
    CacheValidators captured;
    HeaderCapture hdr{&captured.etag, &captured.last_modified};
    curl_easy_setopt(c, CURLOPT_HEADERFUNCTION, CaptureHeader);
    curl_easy_setopt(c, CURLOPT_HEADERDATA, &hdr);
    curl_slist *req_hdrs = BuildConditionalHeaders(in);
    if (req_hdrs) curl_easy_setopt(c, CURLOPT_HTTPHEADER, req_hdrs);
    const CURLcode res = curl_easy_perform(c);
    long http_code = 0;
    curl_easy_getinfo(c, CURLINFO_RESPONSE_CODE, &http_code);
    char *effective_url = nullptr;
    curl_easy_getinfo(c, CURLINFO_EFFECTIVE_URL, &effective_url);
    curl_off_t curl_bytes = 0;
    curl_easy_getinfo(c, CURLINFO_SIZE_DOWNLOAD_T, &curl_bytes);
    curl_slist_free_all(req_hdrs);
    curl_easy_cleanup(c);
    std::fflush(f);
    const long ftell_bytes = std::ftell(f);
    std::fclose(f);
    LOGI("DownloadConditional perform res=%d (%s) http=%ld curl_bytes=%lld ftell=%ld errbuf=%s effective=%s", (int) res, curl_easy_strerror(res), http_code, (long long) curl_bytes, ftell_bytes, errbuf[0] ? errbuf : "(empty)", effective_url ? effective_url : "?");
    if (res != CURLE_OK) {
        r.error = curl_easy_strerror(res);
        if (errbuf[0]) r.error += std::string(" | ") + errbuf;
        RemovePath(tmp, "curl perform failed");
        LOGE("DownloadConditional decision=failed curl_error");
        return r;
    }
    r.status = http_code;
    if (http_code == 304) {
        RemovePath(tmp, "304 not modified");
        if (out) *out = captured;
        LOGI("DownloadConditional decision=304 keep cache path=%s", path.c_str());
        return r;
    }
    if (http_code < 200 || http_code >= 300) {
        r.error = "http " + std::to_string(http_code);
        RemovePath(tmp, "bad http status");
        LOGE("DownloadConditional decision=failed http=%ld keep cache", http_code);
        return r;
    }
    if (ftell_bytes <= 0 || curl_bytes <= 0) {
        r.error = "empty file";
        RemovePath(tmp, "zero bytes written");
        LOGE("DownloadConditional decision=failed empty body");
        return r;
    }
    if (fs::IsFile(path)) fs::Remove(path);
    const auto renamed = fs::RenamePath(tmp, path);
    if (!renamed.ok()) {
        r.error = renamed.error.empty() ? "rename failed" : renamed.error;
        RemovePath(tmp, "rename failed");
        LOGE("DownloadConditional decision=failed rename %s", r.error.c_str());
        return r;
    }
    if (out) *out = captured;
    LOGI("DownloadConditional decision=200 replaced path=%s bytes=%ld etag=%s last-mod=%s", path.c_str(), ftell_bytes, captured.etag.empty() ? "(none)" : captured.etag.c_str(), captured.last_modified.empty() ? "(none)" : captured.last_modified.c_str());
    return r;
}

} // namespace http
