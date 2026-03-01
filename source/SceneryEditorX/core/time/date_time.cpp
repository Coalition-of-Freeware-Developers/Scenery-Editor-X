/**
 * -------------------------------------------------------
 * Scenery Editor X
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray 
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 * -------------------------------------------------------
 * date_time.h
 * -------------------------------------------------------
 * Created: 14/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include "date_time.h"
#include <chrono>
#include <ctime>

// -----------------------------------------------------

namespace SceneryEditorX
{
	using FileTime = std::filesystem::file_time_type;

    DateTime::DateTime()
    {
        const auto now = std::chrono::system_clock::now();
        const time_t timeUtc = std::chrono::system_clock::to_time_t(now);

        timeInfo = *std::localtime(&timeUtc);
    }

    DateTime::~DateTime() = default;

    DateTime::DateTime(const FileTime fileTime)
    {
        //const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
        const auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
        const auto time = std::chrono::system_clock::to_time_t(systemTime);
        timeInfo = *std::localtime(&time);
    }

    DateTime DateTime::Now()
    {
        const auto now = std::chrono::system_clock::now();
        const time_t timeUtc = std::chrono::system_clock::to_time_t(now);

        DateTime result{};
        result.timeInfo = *std::localtime(&timeUtc);

        return result;
    }

    DateTime DateTime::UtcNow()
    {
        const auto now = std::chrono::system_clock::now();
        const time_t timeUtc = std::chrono::system_clock::to_time_t(now);

        DateTime result{};
        result.timeInfo = *std::gmtime(&timeUtc);

        return result;
    }

    void DateTime::AddSeconds(const int seconds)
    {
        if (seconds == 0)
            return;

        timeInfo.tm_sec += seconds;
        std::mktime(&timeInfo);
    }

    void DateTime::AddMinutes(const int minutes)
    {
        if (minutes == 0)
            return;

        timeInfo.tm_min += minutes;
        std::mktime(&timeInfo);
    }

    void DateTime::AddHours(const int hours)
    {
        if (hours == 0)
            return;

        timeInfo.tm_hour += hours;
        std::mktime(&timeInfo);
    }

    void DateTime::AddDays(const int days)
    {
        if (days == 0)
            return;

        timeInfo.tm_mday += days;
        std::mktime(&timeInfo);
    }

    void DateTime::AddMonths(const int months)
    {
        if (months == 0)
            return;

        timeInfo.tm_mon += months;
        std::mktime(&timeInfo);
    }

    void DateTime::AddYears(const int years)
    {
        if (years == 0)
            return;

        timeInfo.tm_year += years;
        std::mktime(&timeInfo);
    }

    uint64_t DateTime::ToNumber() const
    {
        PackedDateTime packed;
        packed.year = Year();
        packed.month = Month();
        packed.day = Day();
        packed.hour = Hour();
        packed.minute = Minute();
        packed.second = Second();

        return packed.finalValue;
    }

    DateTime DateTime::FromNumber(const uint64_t number)
    {
        PackedDateTime packed;
        packed.finalValue = number;

        DateTime result{};
        result.timeInfo.tm_year = static_cast<int>(packed.year) - 1900;
        result.timeInfo.tm_mon = static_cast<int>(packed.month) - 1;
        result.timeInfo.tm_mday = static_cast<int>(packed.day);
        result.timeInfo.tm_hour = static_cast<int>(packed.hour);
        result.timeInfo.tm_min = static_cast<int>(packed.minute);
        result.timeInfo.tm_sec = static_cast<int>(packed.second);

        return result;
    }

    std::string DateTime::ToString() const
    {
        constexpr const char *defaultFormat = "%Y-%m-%d %H:%M:%S";
        std::ostringstream oss;
        oss << std::put_time(&timeInfo, defaultFormat);
        return oss.str();
    }

    DateTime DateTime::Parse(const std::string &input)
    {
        constexpr const char* defaultFormat = "%Y-%m-%d %H:%M:%S";
        DateTime result = {};

        std::istringstream iss(input.c_str());
        iss >> std::get_time(&result.timeInfo, defaultFormat);

        return result;
    }

}

// -----------------------------------------------------
