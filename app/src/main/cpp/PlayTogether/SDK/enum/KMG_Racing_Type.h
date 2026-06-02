//
// Created by TEAMHMG on 04/08/2025.
//

#pragma once
#ifndef PLAY_KMG_RACING_TYPE_H
#define PLAY_KMG_RACING_TYPE_H

enum class KMG_Racing_Type : int {
    None = 0,        // 🔹 Không xác định loại (trạng thái mặc định)
    Obstacle = 1,    // 🔹 Đua vượt chướng ngại vật (Obstacle Course)
    HeroSpeed = 2,   // 🔹 Đua tốc độ cao (Hero Speed Mode)
    Random = 3,      // 🔹 Đua kiểu ngẫu nhiên (random hoán đổi địa hình / vật cản)
    RotationBar = 4, // 🔹 Đua với thanh xoay (người chơi né thanh xoay khi chạy)
    End = 5          // 🔹 Giá trị kết thúc (dùng để giới hạn enum)
};

#endif //PLAY_KMG_RACING_TYPE_H
