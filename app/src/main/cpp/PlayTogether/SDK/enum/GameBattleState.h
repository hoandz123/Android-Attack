//
// Created by DIZENO on 7/30/2025.
//

#ifndef PLAY_GAMEBATTLESTATE_H
#define PLAY_GAMEBATTLESTATE_H

enum class GameBattleState : int {
    None = 0,               // ⚪ Không có trạng thái (mặc định ban đầu)
    BattleStateWaitUser = 1, // ⏳ Chờ người chơi tham gia (đang ghép trận hoặc đợi đủ người)
    BattleStateLoadReady = 2, // 📦 Chuẩn bị load tài nguyên (bắt đầu quá trình tải game)
    BattleStateLoading = 3,   // 💾 Đang tải dữ liệu (bản đồ, nhân vật, hiệu ứng,…)
    BattleStateReady = 4,     // 🟡 Giai đoạn “Ready” – người chơi sẵn sàng, đếm ngược bắt đầu
    BattleStateStart = 5,     // 🟢 Trận đấu đã bắt đầu (đang chơi)
    BattleStateEnd = 6,       // 🔴 Trận đấu kết thúc (đã hết thời gian hoặc có người thắng)
    BattleStateResult = 7,    // 🏁 Hiển thị kết quả (bảng xếp hạng, thắng/thua,…)
    BattleStateReward = 8,    // 🎁 Phát thưởng cho người chơi
    BattleStateWaitAgain = 9, // 🔄 Đợi vòng tiếp theo hoặc rematch
    End = 10                  // 🚫 Kết thúc chuỗi trạng thái (đánh dấu cuối enum)
};

#endif //PLAY_GAMEBATTLESTATE_H
