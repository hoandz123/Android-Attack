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
        {OBF("Quang Truong Plaza"), 1001},
        {OBF("Khu Cam Trai"), 1201},
        {OBF("Khu Dan Cu"), 11501},
        {OBF("Trung Tam Thanh Pho"), 1502},
        {OBF("Dao Nghi Duong"), 1301},
        {OBF("Cua Hang Trang Phuc"), 1101},
        {OBF("Cua Hang Xe Hoi"), 1102},
        {OBF("Cua Hang Noi That"), 1103},
        {OBF("Cua Hang Thu Cung"), 1104},
        {OBF("Seven Eleven"), 1106},
        {OBF("Cong Ngam"), 1401},
        {OBF("Truong Hoc"), 1601},
        {OBF("Sieu Thi Lon"), 1701},
        {OBF("Rap Chieu Phim"), 1702},
        {OBF("Ngan Hang"), 1703},
        {OBF("Don Canh Sat"), 1704},
        {OBF("Toa Thi Chinh"), 1705},
        {OBF("Benh Vien"), 1706},
        {OBF("Khach San"), 1707},
        {OBF("Paldo Koreno"), 1708},
        {OBF("Cua Hang Vani"), 1709},
        {OBF("Trung Tam Du Khach Han Quoc"), 1710},
        {OBF("Bao Tang Nghe Thuat"), 1711},
        {OBF("Ngan Hang Tang 2"), 1712},
        {OBF("Lotteria L Chicken"), 1713},
        {OBF("Khu Tro Choi"), 1714},
        {OBF("Tram Cuu Hoa"), 1715},
        {OBF("Van Phong"), 1716},
    };
}
PLConfig &gPLConfig = GetConfig();

static PLConfig g_config;

PLConfig &GetConfig() {
    return g_config;
}
