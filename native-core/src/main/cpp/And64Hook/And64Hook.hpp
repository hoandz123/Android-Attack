/*
 *  @ngày   : 2018/04/18
 *  @tác giả: Rprop (r_prop@outlook.com)
 *  https://github.com/Rprop/And64InlineHook
 */
/*
 Giấy phép MIT

 Bản quyền (c) 2018 Rprop (r_prop@outlook.com)

 Cho phép bất kỳ ai nhận được bản sao của phần mềm này và các tài liệu liên quan ("Phần mềm"),
 được sử dụng mà không bị hạn chế,
 bao gồm nhưng không giới hạn quyền sử dụng,
 sao chép, sửa đổi, hợp nhất, xuất bản, phân phối,
 cấp phép lại và/hoặc bán các bản sao của Phần mềm,
 và cho phép những người được cung cấp Phần mềm làm như vậy,
 tuân theo các điều kiện sau:

 Thông báo bản quyền ở trên và thông báo cho phép này phải được bao gồm trong tất cả các bản sao hoặc phần lớn của Phần mềm.

 PHẦN MỀM ĐƯỢC CUNG CẤP "NGUYÊN TRẠNG",
 KHÔNG CÓ BẢO ĐẢM NÀO, DÙ RÕ RÀNG HAY NGỤ Ý,
 BAO GỒM NHƯNG KHÔNG GIỚI HẠN Ở CÁC BẢO ĐẢM VỀ KHẢ NĂNG BÁN HÀNG,
 PHÙ HỢP VỚI MỤC ĐÍCH CỤ THỂ VÀ KHÔNG VI PHẠM. TRONG MỌI TRƯỜNG HỢP,
 TÁC GIẢ HOẶC CHỦ SỞ HỮU BẢN QUYỀN KHÔNG CHỊU TRÁCH NHIỆM VỀ BẤT KỲ YÊU CẦU,
 THIỆT HẠI HOẶC TRÁCH NHIỆM NÀO, DÙ TRONG HỢP ĐỒNG, HÀNH VI DÂN SỰ HAY KHÁC,
 PHÁT SINH TỪ, NGOÀI HOẶC LIÊN QUAN ĐẾN PHẦN MỀM HOẶC VIỆC SỬ DỤNG HOẶC CÁC GIAO DỊCH KHÁC TRONG PHẦN MỀM.
 */
#pragma once
#define A64_MAX_BACKUPS 256

#ifdef __cplusplus
extern "C" {
#endif

void A64Hook(void *const symbol, void *const replace, void **result);
void *A64HookV(void *const symbol, void *const replace, void *const rwx, const uintptr_t rwx_size);

#ifdef __cplusplus
}
#endif