//
// Created by DELL on 7/25/2025.
//

#ifndef PLAY_FISHLOGGER_H
#define PLAY_FISHLOGGER_H

#include <cstdint>
#include <json/json.hpp>

struct FishingEntry {
    int64_t      ts      {0};
    int          level   {0};
    std::string  result;
    int          grade   {0};
    int         fishZone {0};
    std::string  name;
    std::string  ymd;
};

class FishLogger {
public:
    explicit FishLogger();
    ~FishLogger() = default;

    static int64_t nowMillis();
    static std::string formatTime(int64_t ms);

    void begin(int grade, int level, int fishZone);
    void markSuccess(int itemType, int itemId, int nameId, const std::string& fishName);
    void markFail();
    void endBegin();
    void Delete();

    std::vector<FishingEntry> LoadHistory(std::vector<std::string>& dates);
private:
    nlohmann::json    record_;
    std::string       logFile_;
    bool              active_{false};
};


#endif //PLAY_FISHLOGGER_H
