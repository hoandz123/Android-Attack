//
// Created by PC on 08/10/2025.
//

#ifndef IL2CPP_PLAY_EBLOCKSTATE_H
#define IL2CPP_PLAY_EBLOCKSTATE_H

enum class eBlockState : int {
    BlockShake = 0,      // 🔹 Sàn đang rung (chuẩn bị rơi hoặc biến mất)
    BlockDown = 1,       // 🔹 Sàn hạ xuống (rơi xuống, không còn an toàn)
    BlockUp = 2,         // 🔹 Sàn nâng lên trở lại (có thể tái sử dụng hoặc hồi phục)
    BlockNotSelect = 3   // 🔹 Sàn chưa được chọn hoặc không hoạt động (vô hiệu hóa trong vòng này)
};

#endif //IL2CPP_PLAY_EBLOCKSTATE_H
