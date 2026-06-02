#include "SharedPrefs.h"
#define LOGGER_TAG "ATTACK_PlayTogether"
#include <Includes/Logger.h>
#include <Tools/Tools.h>

SharedPrefs prefs(OBF("prefs.txt"));

void SharedPrefs::load() {
    static std::string package_name = Tools::GetPackageName();
    std::ifstream file((OBFS("/data/user/0/") + package_name + OBFS("/") + filename));
    if (file.is_open()) {
        std::string key, value;
        while (getline(file, key) && getline(file, value)) {
            data[key] = value;
        }
        file.close();
    }
}

void SharedPrefs::save() {
    static std::string package_name = Tools::GetPackageName();
    std::ofstream file((OBFS("/data/user/0/") + package_name + OBFS("/") + filename));
    if (file.is_open()) {
        for (const auto& pair : data) {
            file << pair.first << std::endl;
            file << pair.second << std::endl;
        }
        file.close();
    }
}

SharedPrefs::SharedPrefs(const std::string& file) : filename(file) {
    load();
}

void SharedPrefs::putString(const std::string& key, const std::string& value) {
    data[key] = value;
    save();
}

std::string SharedPrefs::getString(const std::string& key, const std::string& defaultValue) {
    if (data.find(key) != data.end()) {
        return data[key];
    } else {
        return defaultValue;
    }
}

void SharedPrefs::putInt(const std::string& key, int value) {
    data[key] = std::to_string(value);
    save();
}
int SharedPrefs::getInt(const std::string& key, int defaultValue) {
    if (data.find(key) != data.end()) {
        try {
            return std::stoi(data[key]);
        } catch (...) {
            return defaultValue;
        }
    } else {
        return defaultValue;
    }
}
void SharedPrefs::putBool(const std::string& key, bool value) {
    data[key] = value ? "1" : "0";
    save();
}
bool SharedPrefs::getBool(const std::string& key, bool defaultValue) {
    if (data.find(key) != data.end()) {
        return data[key] == "1";
    } else {
        return defaultValue;
    }
}

void SharedPrefs::remove(const std::string& key) {
    data.erase(key);
    save();
}
