#include "PLConfig.h"
#include "Config.h"
#include <json/json.hpp>
#include <FileManager.hpp>
#include <Tools/Tools.h>
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("ATTACK_PlayTogether")
#include <Includes/Logger.h>
#include <API/Vector3.h>

void to_json(nlohmann::json& j, const Vector3& v) {
    j = nlohmann::json{{"x", v.x}, {"y", v.y}, {"z", v.z}};
}

void from_json(const nlohmann::json& j, Vector3& v) {
    if (j.contains("x")) v.x = j["x"];
    if (j.contains("y")) v.y = j["y"];
    if (j.contains("z")) v.z = j["z"];
}

void to_json(nlohmann::json& j, const PLConfig::NameAndPos& obj) {
    j = nlohmann::json{
        {"name", obj.name},
        {"mapID", obj.mapID},
        {"pos", obj.pos},
        {"isRandom", obj.isRandom}
    };
}

void from_json(const nlohmann::json& j, PLConfig::NameAndPos& obj) {
    if (j.contains("name")) j["name"].get_to(obj.name);
    if (j.contains("mapID")) j["mapID"].get_to(obj.mapID);
    if (j.contains("pos")) j["pos"].get_to(obj.pos);
    if (j.contains("isRandom")) j["isRandom"].get_to(obj.isRandom);
}

void to_json(nlohmann::json& j, const PLConfig::GeneralConfig& cfg) {
    j = nlohmann::json{
        {"isInfo", cfg.isInfo},
        {"isRepair", cfg.isRepair},
        {"isBoQuaLoiThoaiNPC", cfg.isBoQuaLoiThoaiNPC},
        {"isResetTrangThai", cfg.isResetTrangThai},
        {"chaynhanh", cfg.chaynhanh},
        {"nhaycao", cfg.nhaycao}
    };
}

void from_json(const nlohmann::json& j, PLConfig::GeneralConfig& cfg) {
    if (j.contains("isInfo")) j["isInfo"].get_to(cfg.isInfo);
    if (j.contains("isRepair")) j["isRepair"].get_to(cfg.isRepair);
    if (j.contains("isBoQuaLoiThoaiNPC")) j["isBoQuaLoiThoaiNPC"].get_to(cfg.isBoQuaLoiThoaiNPC);
    if (j.contains("isResetTrangThai")) j["isResetTrangThai"].get_to(cfg.isResetTrangThai);
    if (j.contains("chaynhanh")) j["chaynhanh"].get_to(cfg.chaynhanh);
    if (j.contains("nhaycao")) j["nhaycao"].get_to(cfg.nhaycao);
}

void to_json(nlohmann::json& j, const PLConfig& cfg) {
    j = nlohmann::json{
        {"viTriCoSan", cfg.viTriCoSan},
        {"general", cfg.general}
    };
}

void from_json(const nlohmann::json& j, PLConfig& cfg) {
    if (j.contains("viTriCoSan")) j["viTriCoSan"].get_to(cfg.viTriCoSan);
    if (j.contains("general")) j["general"].get_to(cfg.general);
}

static std::string g_config_file_path;

static std::string GetConfigFilePath() {
    try {
        if (!g_config_file_path.empty()) return g_config_file_path;
        std::string packageName = Tools::GetPackageName();
        if (packageName.empty()) packageName = OBF("com.vng.playtogether");
        g_config_file_path = OBF("/storage/emulated/0/Android/data/") + packageName + OBF("/config.json");
        return g_config_file_path;
    } catch (...) {
        return "";
    }
}

void PLConfig::Load(const std::string &content) {
    try {
        auto j = nlohmann::json::parse(content);
        from_json(j, GetConfig());
    } catch (const std::exception &e) {
        LOGE(OBF("PLConfig::Load: %s"), e.what());
    }
}

std::string PLConfig::GetConfigContent() {
    try {
        nlohmann::json j;
        to_json(j, GetConfig());
        return j.dump(4);
    } catch (...) {
        return "{}";
    }
}

bool SaveConfig() {
    try {
        std::string content = GetConfig().GetConfigContent();
        if (content.empty() || content == "{}") return false;
        std::string filePath = GetConfigFilePath();
        if (filePath.empty()) return false;
        fs::Result wr = fs::WriteBytes(filePath, content.data(), content.size());
        return wr.ok();
    } catch (...) {
        return false;
    }
}

bool LoadConfig() {
    try {
        std::string filePath = GetConfigFilePath();
        if (filePath.empty() || !fs::Exists(filePath)) return false;
        fs::Result rr;
        std::vector<uint8_t> bytes = fs::ReadBytes(filePath, &rr);
        if (!rr.ok() || bytes.empty()) return false;
        std::string content(bytes.begin(), bytes.end());
        GetConfig().Load(content);
        return true;
    } catch (...) {
        return false;
    }
}
