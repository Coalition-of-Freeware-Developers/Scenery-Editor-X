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
#include <chrono>
#include <filesystem>

// -----------------------------------------------------

namespace SceneryEditorX
{
	typedef std::filesystem::file_time_type FileTime;

	/**
	 * @class DateTime
	 * @brief 
	 */
	class DateTime final
	{
	public:
		/**
		 * @brief 
		 */
		DateTime();
		~DateTime();

		explicit DateTime(FileTime fileTime);

		/**
		 * @brief 
		 * @return 
		 */
		static DateTime Now();

		/**
		 * @brief 
		 * @return 
		 */
		static DateTime UtcNow();

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] int Second() const { return m_TimeInfo.tm_sec; }

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] int Minute() const { return m_TimeInfo.tm_min; }

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] int Hour() const { return m_TimeInfo.tm_hour; }

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] int Day() const { return m_TimeInfo.tm_mday; }

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] int Month() const { return m_TimeInfo.tm_mon + 1; }

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] int Year() const { return m_TimeInfo.tm_year + 1900; }

		/**
		 * @brief 
		 * @param rhs 
		 * @return 
		 */
		bool operator==(const DateTime& rhs) const
		{
			return Second() == rhs.Second() && Minute() == rhs.Minute() && Hour() == rhs.Hour() &&
				Day() == rhs.Day() && Month() == rhs.Month() && Year() == rhs.Year();
		}

		/**
		 * @brief 
		 * @param rhs 
		 * @return 
		 */
		bool operator!=(const DateTime& rhs) const { return !(*this == rhs); }

		/**
		 * @brief 
		 * @param time 
		 */
		static void SetTimeOfDay(float time);

		/**
		 * @brief 
		 * @param year 
		 * @param month 
		 * @param day 
		 * @param hour 
		 * @param minute 
		 * @param second 
		 */
		static void SetCurrentTime(int year, int month, int day, int hour, int minute, int second);

		/**
		 * @brief 
		 * @param seconds 
		 */
		void AddSeconds(int seconds);

		/**
		 * @brief 
		 * @param minutes 
		 */
		void AddMinutes(int minutes);

		/**
		 * @brief 
		 * @param hours 
		 */
		void AddHours(int hours);

		/**
		 * @brief 
		 * @param days 
		 */
		void AddDays(int days);

		/**
		 * @brief 
		 * @param months 
		 */
		void AddMonths(int months);

		/**
		 * @brief 
		 * @param years 
		 */
		void AddYears(int years);

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] uint64_t ToNumber() const;

		/**
		 * @brief 
		 * @param number 
		 * @return 
		 */
		static DateTime FromNumber(uint64_t number);

		/**
		 * @brief 
		 * @return 
		 */
		[[nodiscard]] std::string ToString() const;

		/**
		 * @brief 
		 * @param input 
		 * @return 
		 */
		static DateTime Parse(const std::string &input);

		/**
		 * @brief 
		 * @return 
		 */
		static DateTime &Instance();

		/**
		 * @brief Validates whether the provided hour, minute, and second values represent a valid time. The method checks that
		 * the hour is between 0 and 23, the minute is between 0 and 59, and the second is between 0 and 59.
		 * @param hour The hour value to validate.
		 * @param minute The minute value to validate.
		 * @param second The
		 * second value to validate.
		 * @return True if the provided values represent a valid time, false otherwise.
		 */
		static bool IsValidTime(int hour, int minute, int second);

	private:
		/**
		 * @struct PackedDateTime
		 * @brief A packed representation of a date and time.
		 */
		struct PackedDateTime
		{
			union
			{
				struct
				{
					uint64_t m_Year		: 16; // 16 bits for year (up to 65535)
					uint64_t m_Month	: 4;  // 4 bits for month (up to 12)
					uint64_t m_Day		: 5;  // 5 bits for day (up to 31)
					uint64_t m_Hour		: 5;  // 5 bits for hour (up to 23)
					uint64_t m_Minute	: 6;  // 6 bits for minute (up to 60)
					uint64_t m_Second	: 6;  // 6 bits for second (up to 60)
				};

				uint64_t m_FinalValue = 0;  // The packed value as a single 64-bit integer
			};
		};

		std::tm m_TimeInfo{};
	};
}

// -----------------------------------------------------
