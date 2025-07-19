/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* date_time.h
* -------------------------------------------------------
* Created: 14/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <chrono>
#include <filesystem>
#include <fstream>

/// -----------------------------------------------------

namespace SceneryEditorX
{
	using FileTime = std::filesystem::file_time_type;

    class DateTime final
    {
    public:

        DateTime();
        ~DateTime();

        DateTime(FileTime fileTime);

        static DateTime Now();

        static DateTime UtcNow();

        inline int Second() const { return timeInfo.tm_sec; }
        inline int Minute() const { return timeInfo.tm_min; }
        inline int Hour() const { return timeInfo.tm_hour; }
        inline int Day() const { return timeInfo.tm_mday; }
        inline int Month() const { return timeInfo.tm_mon + 1; }
        inline int Year() const { return timeInfo.tm_year + 1900; }

        inline bool operator==(const DateTime& rhs) const
        {
            return Second() == rhs.Second() && Minute() == rhs.Minute() && Hour() == rhs.Hour() &&
                Day() == rhs.Day() && Month() == rhs.Month() && Year() == rhs.Year();
        }

        inline bool operator!=(const DateTime& rhs) const { return !(*this == rhs); }

        void AddSeconds(int seconds);
        void AddMinutes(int minutes);
        void AddHours(int hours);
        void AddDays(int days);
        void AddMonths(int months);
        void AddYears(int years);

        uint64_t ToNumber() const;
        static DateTime FromNumber(uint64_t number);

        std::string ToString() const;
        static DateTime Parse(const std::string &input);

    private:

        struct PackedDateTime
        {
            union {
                struct {
                    uint64_t year : 16;   // 16 bits for year (up to 65535)
                    uint64_t month : 4;    // 4 bits for month (up to 12)
                    uint64_t day : 5;    // 5 bits for day (up to 31)
                    uint64_t hour : 5;    // 5 bits for hour (up to 23)
                    uint64_t minute : 6;    // 6 bits for minute (up to 60)
                    uint64_t second : 6;    // 6 bits for second (up to 60)
                };

                uint64_t finalValue = 0;
            };
        };

        std::tm timeInfo{};
    };
}

/// -----------------------------------------------------
