#ifndef SHAREDPREFS_H
#define SHAREDPREFS_H

#include <string>
#include <fstream>
#include <unordered_map>
class SharedPrefs {
private:
    std::unordered_map<std::string, std::string> data;
    std::string filename;
    void load();
    void save();

public:
    SharedPrefs(const std::string& file);
    void putString(const std::string& key, const std::string& value);
    std::string getString(const std::string& key, const std::string& defaultValue = "");
    void putInt(const std::string& key, int value);
    int getInt(const std::string& key, int defaultValue = 0);
    void putBool(const std::string& key, bool value);
    bool getBool(const std::string& key, bool defaultValue = false);
    void remove(const std::string& key);
};
extern SharedPrefs prefs;
#endif // SHAREDPREFS_H
