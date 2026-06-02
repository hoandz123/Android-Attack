//
// Created by Administrator on 08/12/2024.
//

#include "SharedPrefs.h"
#include <Includes/obfuscate.h>
#define LOG_TAG OBF("AttackPlugin")
#include <Includes/Logger.h>
#include <Tools/Tools.h>

SharedPrefs prefs(OBF("prefs.txt"));



void SharedPrefs::load() {
    static std::string package_name = Tools::GetPackageName();
//    LOGD("Package name: %s", package_name.c_str());
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

// Constructor nhận vào tên file
SharedPrefs::SharedPrefs(const std::string& file) : filename(file) {
    load();  // Khi tạo đối tượng, đọc dữ liệu từ file
}

// Lưu một giá trị văn bản với một khóa
void SharedPrefs::putString(const std::string& key, const std::string& value) {
    data[key] = value;
    save();  // Sau khi thay đổi, lưu lại vào file
}

std::string SharedPrefs::getString(const std::string& key, const std::string& defaultValue) {
    if (data.find(key) != data.end()) {
//        LOGD("Key: %s, Value: %s", key.c_str(), data[key].c_str());
        return data[key];
    } else {
        return defaultValue;
    }
}

// Lưu một giá trị số nguyên với một khóa
void SharedPrefs::putInt(const std::string& key, int value) {
    data[key] = std::to_string(value);
    save();  // Sau khi thay đổi, lưu lại vào file
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
// Lưu một giá trị boolean với một khóa
void SharedPrefs::putBool(const std::string& key, bool value) {
    data[key] = value ? "1" : "0";
    save();  // Sau khi thay đổi, lưu lại vào file
}
bool SharedPrefs::getBool(const std::string& key, bool defaultValue) {
    if (data.find(key) != data.end()) {
        return data[key] == "1";
    } else {
        return defaultValue;
    }
}

// Xóa một khóa
void SharedPrefs::remove(const std::string& key) {
    data.erase(key);
    save();  // Sau khi xóa, lưu lại vào file
}
