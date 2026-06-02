//
// Created by TEAMHMG on 04/08/2025.
//

#pragma once
#ifndef PLAY_KNOCKOUTMINIGAME_TYPE_H
#define PLAY_KNOCKOUTMINIGAME_TYPE_H

enum class KnockOutMiniGame_Type {
    None = 0,          // Không có mini game
    FloorRandom = 1,   // Mini game: Sàn ngẫu nhiên (người chơi đứng trên sàn biến mất ngẫu nhiên)
    FloorPaint = 2,    // Mini game: Tô màu sàn (người chơi sơn màu ô sàn)
    Racing = 3,        // Mini game: Đua (racing)
    StealCrown = 4,    // Mini game: Cướp vương miện
    PassBomb = 5,      // Mini game: Chuyền bom (tránh bị nổ)
    Duel = 6,          // Mini game: Đấu tay đôi
    GreenRedLight = 7, // Mini game: Đèn xanh đèn đỏ (di chuyển khi đèn xanh, dừng khi đèn đỏ)
    RealBoard = 8,     // Mini game: Bảng thực tế (có thể là trò chơi trên bảng thực)
    Mingle = 9,        // Mini game: Hòa nhập (người chơi tương tác với nhau)
    End = 10           // Kết thúc danh sách mini game
};
#endif //PLAY_KNOCKOUTMINIGAME_TYPE_H
