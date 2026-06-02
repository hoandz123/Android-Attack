#include "Config.h"
#include "SDK/CacheUser.h"
#include "SDK/KinematicCharacterMotor.h"
#include <Includes/obfuscate.h>

bool isGameLoading = false;

int PLConfig::GetPlayerMapID() {
    return CacheUser::myCurrentMapID();
}

Vector3 PLConfig::GetPlayerPosition() {
    return KinematicCharacterMotor::get_TransientPosition();
}

std::vector<PLConfig::MapInfo> PLConfig::GetMapInfoList() {
    return {
        {OBF("Quảng trường"), 1001},
        {OBF("Khu cắm trại"), 1201},
        {OBF("Khu dân cư"), 11501},
        {OBF("Trung tâm TP"), 1502},
        {OBF("Đảo nghỉ dưỡng"), 1301},
        {OBF("Cửa hàng trang phục"), 1101},
        {OBF("Cửa hàng xe"), 1102},
        {OBF("Cửa hàng nội thất"), 1103},
        {OBF("Cửa hàng thú cưng"), 1104},
        {OBF("Seven Eleven"), 1106},
        {OBF("Cống ngầm"), 1401},
        {OBF("Trường học"), 1601},
        {OBF("Siêu thị lớn"), 1701},
        {OBF("Rạp chiếu phim"), 1702},
        {OBF("Ngân hàng"), 1703},
        {OBF("Đồn cảnh sát"), 1704},
        {OBF("Tòa thị chính"), 1705},
        {OBF("Bệnh viện"), 1706},
        {OBF("Khách sạn"), 1707},
        {OBF("Paldo Koreno"), 1708},
        {OBF("Cửa hàng Vani"), 1709},
        {OBF("TT du khách HQ"), 1710},
        {OBF("Bảo tàng nghệ thuật"), 1711},
        {OBF("Ngân hàng tầng 2"), 1712},
        {OBF("Lotteria L Chicken"), 1713},
        {OBF("Khu trò chơi"), 1714},
        {OBF("Trạm cứu hỏa"), 1715},
        {OBF("Văn phòng"), 1716},
    };
}
PLConfig &gPLConfig = GetConfig();

static PLConfig g_config;

PLConfig &GetConfig() {
    return g_config;
}
