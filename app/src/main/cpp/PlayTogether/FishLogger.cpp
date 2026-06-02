#include "PlayLog.h"
#include "FishLogger.h"
#include <chrono>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <Tools/Tools.h>
#include <FileManager/FileManager.hpp>
#include <json/json.hpp>

FishLogger::FishLogger() {
    std::string pkg = Tools::GetPackageName();
    logFile_ = OBFS("/data/data/") + pkg + OBFS("/files/fishing.json");
}

int64_t FishLogger::nowMillis() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

std::string FishLogger::formatTime(int64_t ms) {
    std::time_t t = static_cast<time_t>(ms / 1000);
    char buf[32];
    std::strftime(buf, sizeof(buf), OBF("%F %T"), std::localtime(&t));
    return buf;
}

void FishLogger::begin(int grade, int level, int fishZone) {
    LOGI(OBF("begin(%d)"), level);
    record_.clear();
    record_[OBFS("grade")] = grade;
    record_[OBFS("level")] = level;
    record_[OBFS("fishZone")] = fishZone;
    record_[OBFS("name")] = OBFS("Chưa xác định");
    record_[OBFS("ts")] = nowMillis();
    active_ = true;
}

void FishLogger::markSuccess(int itemType, int itemId, int nameId, const std::string &fishName) {
    LOGI(OBF("markSuccess(%d, %d, %d, %s)"), itemType, itemId, nameId, fishName.c_str());
    if (!active_) return;
    record_[OBFS("result")] = OBFS("success");
    record_[OBFS("ItemType")] = itemType;
    record_[OBFS("ItemId")] = itemId;
    record_[OBFS("NameId")] = nameId;
    record_[OBFS("name")] = fishName;
    std::string line = record_.dump();
    fs::AppendBytes(logFile_, line.data(), line.size());
    active_ = false;
}

void FishLogger::markFail() {
    LOGI(OBF("markFail()"));
    if (!active_) return;
    record_[OBFS("result")] = OBFS("fail");
    std::string line = record_.dump();
    fs::AppendBytes(logFile_, line.data(), line.size());
    active_ = false;
}

void FishLogger::endBegin() {
    LOGI(OBF("endBegin()"));
    active_ = false;
}

void FishLogger::Delete() {
    fs::Remove(logFile_);
}

std::vector<FishingEntry> FishLogger::LoadHistory(std::vector<std::string> &dates) {
    std::vector<FishingEntry> out;
    dates.clear();
    dates.push_back(OBF("Tất cả"));
    if (!fs::Exists(logFile_)) return out;
    auto bytes = fs::ReadBytes(logFile_);
    std::string content(bytes.begin(), bytes.end());
    std::istringstream iss(content);
    std::string line, lastDate;
    while (std::getline(iss, line)) {
        if (line.empty()) continue;
        auto j = nlohmann::json::parse(line, nullptr, false);
        if (j.is_discarded()) continue;
        try {
            FishingEntry e;
            e.ts = j.value(OBFS("ts"), 0LL);
            e.level = j.value(OBFS("level"), 0);
            e.result = j.value(OBFS("result"), OBFS(""));
            e.grade = j.value(OBFS("grade"), 0);
            e.name = j.value(OBFS("name"), OBFS(""));
            e.fishZone = j.value(OBFS("fishZone"), 0);
            e.ymd = formatTime(e.ts).substr(0, 10);
            if (e.ymd != lastDate) {
                dates.push_back(e.ymd);
                lastDate = e.ymd;
            }
            out.emplace_back(std::move(e));
        } catch (const std::exception &ex) {
            LOGE(OBF("Error parsing JSON: %s"), ex.what());
        }
    }
    return out;
}
