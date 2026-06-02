// DateTime.h
//
// Created by TEAMHMG on 26/07/2025.
//

#pragma once
#ifndef PLAY_DATETIME_H
#define PLAY_DATETIME_H

#include <cstdint>
#include <ctime>
#include "API/Il2CppApi.h"

namespace System {

    struct TimeSpan {
        int64_t _ticks;

        TimeSpan(int64_t ticks = 0) : _ticks(ticks) {}

        double totalMilliseconds() const {
            return _ticks / 10000.0;
        }

        double totalSeconds() const {
            return _ticks / 10000000.0;
        }

        double totalMinutes() const {
            return _ticks / 600000000.0;
        }

        double totalHours() const {
            return _ticks / 36000000000.0;
        }

        double totalDays() const {
            return _ticks / 864000000000.0;
        }
    };

    struct DateTime {
    public:
        uint64_t dateData;

        // Static properties
        static DateTime Now();
        static DateTime UtcNow();
        static DateTime Today();
        static DateTime MinValue();
        static DateTime MaxValue();

        // Instance properties
        int64_t GetTicks() const;
        int32_t GetYear() const;
        int32_t GetMonth() const;
        int32_t GetDay() const;
        int32_t GetHour() const;
        int32_t GetMinute() const;
        int32_t GetSecond() const;
        int32_t GetMillisecond() const;
        int32_t GetKind() const;

        // Operations
        DateTime Add(const TimeSpan& value) const;
        DateTime AddTicks(int64_t ticks) const;
        DateTime AddYears(int32_t years) const;
        DateTime AddMonths(int32_t months) const;
        DateTime AddDays(double days) const;
        DateTime AddHours(double hours) const;
        DateTime AddMinutes(double minutes) const;
        DateTime AddSeconds(double seconds) const;
        DateTime AddMilliseconds(double milliseconds) const;
        TimeSpan Subtract(const DateTime& value) const;
        DateTime Subtract(const TimeSpan& value) const;

        // Comparisons
        bool operator==(const DateTime& other) const;
        bool operator!=(const DateTime& other) const;
        bool operator<(const DateTime& other) const;
        bool operator<=(const DateTime& other) const;
        bool operator>(const DateTime& other) const;
        bool operator>=(const DateTime& other) const;

        // Conversion
        String* ToString() const;
        String* ToString(String* format) const;
        DateTime ToUniversalTime() const;
        DateTime ToLocalTime() const;

        // Parse
        static DateTime Parse(String* s);
    };

} // namespace System

#endif //PLAY_DATETIME_H