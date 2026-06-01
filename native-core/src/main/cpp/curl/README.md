# cURL (vendored)

Prebuilt from vvb2060 curl 8.18.0 + boringssl 20251124, merged per ABI into **`libcurl_all.a`**
(curl + nghttp2 + nghttp3 + ngtcp2 + ngtcp2_crypto + ssl + crypto).

Headers: `include/curl/`. Link `native-core::curl` + system `libz` (like `native-core::dobby`).
