#pragma once
#include <asm/unistd.h>
#include <cstdint>
#include <cstring>
#include <errno.h>
#include <sys/syscall.h>
#include <sys/uio.h>
#include <unistd.h>

namespace il2cpp_api {
namespace detail {

// Đọc bytes từ một VA của process hiện tại an toàn (không crash trên page invalid).
// Trả về true nếu đọc đủ len byte, false nếu fail (EFAULT/ENOSYS/lỗi khác).
// Internal: dùng process_vm_readv với pid = getpid(). Nếu syscall không khả dụng
// (ENOSYS trên kernel rất cũ), fallback sang memcpy raw deref — chấp nhận rủi ro
// crash (đó là behavior cũ; nếu kernel hỗ trợ thì có thêm safety).
inline bool safe_read_va(uintptr_t va, void *dst, size_t len) {
    if (!dst || len == 0) return false;
    struct iovec local{dst, len};
    struct iovec remote{reinterpret_cast<void *>(va), len};
    const pid_t pid = getpid();
    const ssize_t n = static_cast<ssize_t>(
        syscall(__NR_process_vm_readv, pid, &local, 1UL, &remote, 1UL, 0UL));
    if (n == static_cast<ssize_t>(len)) return true;
    if (n >= 0) return false;  // partial read
    // errno set; nếu ENOSYS fallback memcpy (kernel quá cũ)
    if (errno == ENOSYS) {
        std::memcpy(dst, reinterpret_cast<const void *>(va), len);
        return true;
    }
    return false;
}

// Đọc C-string an toàn, dừng tại NUL, max_len bao gồm NUL.
// Trả về số byte đã đọc trước NUL (không tính NUL). 0 nếu fail.
inline size_t safe_read_cstr_bounded(uintptr_t va, char *dst, size_t max_len) {
    if (!dst || max_len == 0) return 0;
    size_t total = 0;
    while (total < max_len) {
        size_t chunk = max_len - total;
        if (chunk > 64) chunk = 64;
        if (!safe_read_va(va + total, dst + total, chunk)) return 0;
        for (size_t i = 0; i < chunk; ++i) {
            if (dst[total + i] == '\0') {
                return total + i;
            }
        }
        total += chunk;
    }
    if (max_len > 0) dst[max_len - 1] = '\0';
    return max_len - 1;
}

}  // namespace detail
}  // namespace il2cpp_api
