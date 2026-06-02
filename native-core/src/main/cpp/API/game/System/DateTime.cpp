// DateTime.cpp
//
// Created by TEAMHMG on 26/07/2025.
//

#include "DateTime.h"

namespace System {

    // Static properties
    DateTime DateTime::Now() {
        static auto _ = (DateTime (*)()) GET_METHOD("System", "DateTime", "get_Now", 0);
        return _ ();
    }
    DateTime DateTime::UtcNow() {
        static auto _ = (DateTime (*)()) GET_METHOD("System", "DateTime", "get_UtcNow", 0);
        return _ ();
    }
    DateTime DateTime::Today() {
        static auto _ = (DateTime (*)()) GET_METHOD("System", "DateTime", "get_Today", 0);
        return _ ();
    }
    DateTime DateTime::MinValue() {
        static auto _ = (DateTime (*)()) GET_METHOD("System", "DateTime", "get_MinValue", 0);
        return _ ();
    }
    DateTime DateTime::MaxValue() {
        static auto _ = (DateTime (*)()) GET_METHOD("System", "DateTime", "get_MaxValue", 0);
        return _ ();
    }

    // Instance properties
    int64_t DateTime::GetTicks() const {
        static auto _ = (int64_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Ticks", 0);
        return _ (this);
    }
    int32_t DateTime::GetYear() const {
        static auto _ = (int32_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Year", 0);
        return _ (this);
    }
    int32_t DateTime::GetMonth() const {
        static auto _ = (int32_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Month", 0);
        return _ (this);
    }
    int32_t DateTime::GetDay() const {
        static auto _ = (int32_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Day", 0);
        return _ (this);
    }
    int32_t DateTime::GetHour() const {
        static auto _ = (int32_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Hour", 0);
        return _ (this);
    }
    int32_t DateTime::GetMinute() const {
        static auto _ = (int32_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Minute", 0);
        return _ (this);
    }
    int32_t DateTime::GetSecond() const {
        static auto _ = (int32_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Second", 0);
        return _ (this);
    }
    int32_t DateTime::GetMillisecond() const {
        static auto _ = (int32_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Millisecond", 0);
        return _ (this);
    }
    int32_t DateTime::GetKind() const {
        static auto _ = (int32_t (*)(const DateTime*)) GET_METHOD("System", "DateTime", "get_Kind", 0);
        return _ (this);
    }

    // Operations
    DateTime DateTime::Add(const TimeSpan& value) const {
        static auto _ = (DateTime (*)(const DateTime*, TimeSpan)) GET_METHOD("System", "DateTime", "Add", 1);
        return _ (this, value);
    }
    DateTime DateTime::AddTicks(int64_t ticks) const {
        static auto _ = (DateTime (*)(const DateTime*, int64_t)) GET_METHOD("System", "DateTime", "AddTicks", 1);
        return _ (this, ticks);
    }
    DateTime DateTime::AddYears(int32_t years) const {
        static auto _ = (DateTime (*)(const DateTime*, int32_t)) GET_METHOD("System", "DateTime", "AddYears", 1);
        return _ (this, years);
    }
    DateTime DateTime::AddMonths(int32_t months) const {
        static auto _ = (DateTime (*)(const DateTime*, int32_t)) GET_METHOD("System", "DateTime", "AddMonths", 1);
        return _ (this, months);
    }
    DateTime DateTime::AddDays(double days) const {
        static auto _ = (DateTime (*)(const DateTime*, double)) GET_METHOD("System", "DateTime", "AddDays", 1);
        return _ (this, days);
    }
    DateTime DateTime::AddHours(double hours) const {
        static auto _ = (DateTime (*)(const DateTime*, double)) GET_METHOD("System", "DateTime", "AddHours", 1);
        return _ (this, hours);
    }
    DateTime DateTime::AddMinutes(double minutes) const {
        static auto _ = (DateTime (*)(const DateTime*, double)) GET_METHOD("System", "DateTime", "AddMinutes", 1);
        return _ (this, minutes);
    }
    DateTime DateTime::AddSeconds(double seconds) const {
        static auto _ = (DateTime (*)(const DateTime*, double)) GET_METHOD("System", "DateTime", "AddSeconds", 1);
        return _ (this, seconds);
    }
    DateTime DateTime::AddMilliseconds(double milliseconds) const {
        static auto _ = (DateTime (*)(const DateTime*, double)) GET_METHOD("System", "DateTime", "AddMilliseconds", 1);
        return _ (this, milliseconds);
    }
    TimeSpan DateTime::Subtract(const DateTime& value) const {
        static auto _ = (TimeSpan (*)(const DateTime*, DateTime)) GET_METHOD("mscorlib.dll", "System", "DateTime", "Subtract", 1, 1);
        return _ (this, value);
    }
    DateTime DateTime::Subtract(const TimeSpan& value) const {
        static auto _ = (DateTime (*)(const DateTime*, TimeSpan)) GET_METHOD("mscorlib.dll", "System", "DateTime", "Subtract", 1, 2);
        return _ (this, value);
    }

    bool DateTime::operator==(const DateTime& other) const {
        return this->GetTicks() == other.GetTicks();
    }

    bool DateTime::operator!=(const DateTime& other) const {
        return this->GetTicks() != other.GetTicks();
    }

    bool DateTime::operator<(const DateTime& other) const {
        return this->GetTicks() < other.GetTicks();
    }

    bool DateTime::operator<=(const DateTime& other) const {
        return this->GetTicks() <= other.GetTicks();
    }

    bool DateTime::operator>(const DateTime& other) const {
        return this->GetTicks() > other.GetTicks();
    }

    bool DateTime::operator>=(const DateTime& other) const {
        return this->GetTicks() >= other.GetTicks();
    }


    // Conversion
    String* DateTime::ToString() const {
        static auto _ = (String* (*)(const DateTime*)) GET_METHOD("System", "DateTime", "ToString", 0);
        return _ (this);
    }
    String* DateTime::ToString(String* format) const {
        static auto _ = (String* (*)(const DateTime*, String*)) GET_METHOD("System", "DateTime", "ToString", 1);
        return _ (this, format);
    }
    DateTime DateTime::ToUniversalTime() const {
        static auto _ = (DateTime (*)(const DateTime*)) GET_METHOD("System", "DateTime", "ToUniversalTime", 0);
        return _ (this);
    }
    DateTime DateTime::ToLocalTime() const {
        static auto _ = (DateTime (*)(const DateTime*)) GET_METHOD("System", "DateTime", "ToLocalTime", 0);
        return _ (this);
    }

    // Parse
    DateTime DateTime::Parse(String* s) {
        static auto _ = (DateTime (*)(String*)) GET_METHOD("System", "DateTime", "Parse", 1);
        return _ (s);
    }

} // namespace System