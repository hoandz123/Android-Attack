//
// Created by HMGTEAM on 11/05/2025.
//

#ifndef PLAY_PRO_MAX_EFISHINGSTATE_H
#define PLAY_PRO_MAX_EFISHINGSTATE_H
enum class eFishingState {
    None = 0,                // Không ở trạng thái câu cá nào
    Casting = 1,             // Đang quăng cần
    Search = 2,              // Tìm kiếm cá
    SearchResult = 3,        // Có kết quả tìm kiếm (phát hiện cá)
    Idle = 4,                // Đang chờ (thụ động)
    Hit = 5,                 // Cá cắn câu
    Fighting = 6,            // Đang chiến đấu với cá
    Catch = 7,               // Bắt cá thành công
    Fail = 8,                // Thất bại (mất cá)
    Boast = 9,               // Khoe cá
    Finish = 10,             // Hoàn tất quá trình câu
    CastingFail = 11,        // Quăng cần thất bại
    Miss = 12,               // Bỏ lỡ cá
    BigFish_RaidEnter = 13,  // Vào trận săn cá lớn (Raid)
    BigFish_RaidSync = 14,   // Đồng bộ trạng thái Raid cá lớn
    BigFish_Begin = 15,      // Bắt đầu bắt cá lớn
    BigFish_Pumpin = 16,     // Hành động "Pump in" (kéo cần mạnh để thu dây)
    BigFish_Drag = 17,       // Đang kéo cá lớn
    BigFish_Tug = 18,        // Giật cần khi cá lớn cắn
    BigFish_Fighting = 19,   // Đang chiến đấu với cá lớn
    BigFish_Catch = 20,      // Bắt cá lớn thành công
    BigFish_Miss = 21,       // Bỏ lỡ cá lớn
    BigFish_RaidFighting = 22,// Đang chiến đấu Raid cá lớn
    BigFish_StunBegin = 23,  // Bắt đầu làm cá lớn bị choáng
    BigFish_Stun = 24,       // Cá lớn đang bị choáng
    BigFish_StunRecovery = 25// Cá lớn hồi phục sau khi choáng
};

#endif //PLAY_PRO_MAX_EFISHINGSTATE_H
