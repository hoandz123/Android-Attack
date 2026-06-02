//
// Created by TEAMHMG on 04/08/2025.
//

#pragma once
#ifndef PLAY_KMG_GAME_TYPE_H
#define PLAY_KMG_GAME_TYPE_H

enum class KMG_Game_Type : int {
    None = 0,        // ⚪ Không xác định loại game (mặc định, chưa khởi tạo)
    Solo = 1,        // 🧍‍♂️ Chơi đơn — mỗi người chơi thi đấu độc lập
    Team = 2,        // 👥 Chơi theo đội — chia nhóm thi đấu (đỏ / xanh / vàng, v.v.)
    Asymmetric = 3,  // ⚖️ Chơi bất đối xứng — 1 phe đối đầu nhiều phe (ví dụ: “1 làm boss vs nhiều người”)
    End = 4          // 🚩 Giá trị kết thúc enum (đánh dấu giới hạn)
};

#endif //PLAY_KMG_GAME_TYPE_H
