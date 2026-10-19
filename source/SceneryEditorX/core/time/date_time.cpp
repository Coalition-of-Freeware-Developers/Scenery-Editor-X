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

		m_TimeInfo = *std::localtime(&timeUtc);
	}

	DateTime::~DateTime() = default;

	DateTime::DateTime(const FileTime fileTime)
	{
		//const auto systemTime = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
		const auto systemTime = std::chrono::time_point_cast<std::chrono::system_clock::duration>(fileTime - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());
		const auto time = std::chrono::system_clock::to_time_t(systemTime);
		m_TimeInfo = *std::localtime(&time);
	}

	DateTime DateTime::Now()
	{
		const auto now = std::chrono::system_clock::now();
		const time_t timeUtc = std::chrono::system_clock::to_time_t(now);

		DateTime result{};
		result.m_TimeInfo = *std::localtime(&timeUtc);

		return result;
	}

	DateTime DateTime::UtcNow()
	{
		const auto now = std::chrono::system_clock::now();
		const time_t timeUtc = std::chrono::system_clock::to_time_t(now);

		DateTime result{};
		result.m_TimeInfo = *std::gmtime(&timeUtc);

		return result;
	}

	void DateTime::SetTimeOfDay(float time)
	{
		if (time < 0.0f)
		{
			time = 0.0f;
		}
		else if (time > 1.0f)
		{
			time = 1.0f;
		}
		
		DateTime& instance = Instance();
		/*
		if (instance.Year() <= 0 || instance.Month() <= 0 || instance.Day() <= 0)
		{
			SetCurrentTime();
		}
		*/
		
		int hour              = static_cast<int>(time * 24.0f);
		float minute_fraction = time * 24.0f - static_cast<float>(hour);
		int minute            = static_cast<int>(minute_fraction * 60.0f);
		float second_fraction = minute_fraction * 60.0f - static_cast<float>(minute);
		int second            = static_cast<int>(second_fraction * 60.0f);
		//SetCurrentTime(instance.m_current_year, static_cast<int>(instance.m_current_month), instance.m_current_day, hour, minute, second);
	}

	void DateTime::SetCurrentTime(const int year, const int month, const int day, const int hour, const int minute, const int second)
	{

		/*
		if (!Calendar::IsValidDate(year, month, day) || !IsValidTime(hour, minute, second))
		{
			return;
		}

		DateTime& instance        = Instance();
		instance.m_current_year   = year;
		instance.m_current_month  = static_cast<float>(month);
		instance.m_current_day    = day;
		instance.m_current_hour   = static_cast<float>(hour);
		instance.m_current_minute = minute;
		instance.m_current_second = second;
		instance.m_time_of_day    = (static_cast<float>(hour) + minute / 60.0f + second / 3600.0f) / 24.0f;

		tm local_tm{};
		local_tm.tm_year   = year - 1900;
		local_tm.tm_mon    = month - 1;
		local_tm.tm_mday   = day;
		local_tm.tm_hour   = hour;
		local_tm.tm_min    = minute;
		local_tm.tm_sec    = second;
		time_t target_time = to_utc_time_t(local_tm, TimeZone::GetTimeZoneOffsetHours());
		if (target_time == -1)
		{
			return;
		}

		auto target_tp = std::chrono::system_clock::from_time_t(target_time);
		auto offset    = std::chrono::duration_cast<std::chrono::seconds>(target_tp - std::chrono::system_clock::now());
		Calendar::SetCurrentTimeOffset(offset);
		*/
	}

	void DateTime::AddSeconds(const int seconds)
	{
		if (seconds == 0)
			return;

		m_TimeInfo.tm_sec += seconds;
		std::mktime(&m_TimeInfo);
	}

	void DateTime::AddMinutes(const int minutes)
	{
		if (minutes == 0)
			return;

		m_TimeInfo.tm_min += minutes;
		std::mktime(&m_TimeInfo);
	}

	void DateTime::AddHours(const int hours)
	{
		if (hours == 0)
			return;

		m_TimeInfo.tm_hour += hours;
		std::mktime(&m_TimeInfo);
	}

	void DateTime::AddDays(const int days)
	{
		if (days == 0)
			return;

		m_TimeInfo.tm_mday += days;
		std::mktime(&m_TimeInfo);
	}

	void DateTime::AddMonths(const int months)
	{
		if (months == 0)
			return;

		m_TimeInfo.tm_mon += months;
		std::mktime(&m_TimeInfo);
	}

	void DateTime::AddYears(const int years)
	{
		if (years == 0)
			return;

		m_TimeInfo.tm_year += years;
		std::mktime(&m_TimeInfo);
	}

	uint64_t DateTime::ToNumber() const
	{
		PackedDateTime packed;
		packed.m_Year = Year();
		packed.m_Month = Month();
		packed.m_Day = Day();
		packed.m_Hour = Hour();
		packed.m_Minute = Minute();
		packed.m_Second = Second();

		return packed.m_FinalValue;
	}

	DateTime DateTime::FromNumber(const uint64_t number)
	{
		PackedDateTime packed;
		packed.m_FinalValue = number;

		DateTime result{};
		result.m_TimeInfo.tm_year = static_cast<int>(packed.m_Year) - 1900;
		result.m_TimeInfo.tm_mon = static_cast<int>(packed.m_Month) - 1;
		result.m_TimeInfo.tm_mday = static_cast<int>(packed.m_Day);
		result.m_TimeInfo.tm_hour = static_cast<int>(packed.m_Hour);
		result.m_TimeInfo.tm_min = static_cast<int>(packed.m_Minute);
		result.m_TimeInfo.tm_sec = static_cast<int>(packed.m_Second);

		return result;
	}

	std::string DateTime::ToString() const
	{
		constexpr const char *defaultFormat = "%Y-%m-%d %H:%M:%S";
		std::ostringstream oss;
		oss << std::put_time(&m_TimeInfo, defaultFormat);
		return oss.str();
	}

	DateTime DateTime::Parse(const std::string &input)
	{
		constexpr const char* defaultFormat = "%Y-%m-%d %H:%M:%S";
		DateTime result = {};

		std::istringstream iss(input.c_str());
		iss >> std::get_time(&result.m_TimeInfo, defaultFormat);

		return result;
	}

	DateTime& DateTime::Instance()
	{
		static DateTime instance;
		return instance;
	}

	bool DateTime::IsValidTime(const int hour, const int minute, const int second)
	{
		SEDX_CORE_ASSERT(hour >= 0 && hour <= 23, "Hour must be in range 0-23");
		SEDX_CORE_ASSERT(minute >= 0 && minute <= 59, "Minute must be in range 0-59");
		SEDX_CORE_ASSERT(second >= 0 && second <= 59, "Second must be in range 0-59");

		if (hour < 0 || hour > 23)
		{
			return false;
		}

		if (minute < 0 || minute > 59)
		{
			return false;
		}

		if (second < 0 || second > 59)
		{
			return false;
		}

		return true;
	}

	} // namespace SceneryEditorX

// -----------------------------------------------------
