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
		 * @brief Constructs a DateTime instance representing the current date and time.
		 */
		DateTime();
		~DateTime();

		/**
		 * @brief Constructs a DateTime instance from a file time.
		 * @param fileTime The file time to initialize the DateTime instance with.
		 */
		explicit DateTime(FileTime fileTime);

		/**
		 * @brief Returns a DateTime instance representing the current date and time.
		 * @return A DateTime instance representing the current date and time.
		 */
		static DateTime Now();

		/**
		 * @brief Returns a DateTime instance representing the current date and time in UTC.
		 * @return A DateTime instance representing the current date and time in UTC.
		 */
		static DateTime UtcNow();

		/**
		 * @brief Returns the second component of the DateTime instance.
		 * @return The second component of the DateTime instance.
		 */
		[[nodiscard]] int Second() const { return m_TimeInfo.tm_sec; }

		/**
		 * @brief Returns the minute component of the DateTime instance.
		 * @return The minute component of the DateTime instance.
		 */
		[[nodiscard]] int Minute() const { return m_TimeInfo.tm_min; }

		/**
		 * @brief Returns the hour component of the DateTime instance. 
		 * @return The hour component of the DateTime instance.
		 */
		[[nodiscard]] int Hour() const { return m_TimeInfo.tm_hour; }

		/**
		 * @brief Returns the day component of the DateTime instance.
		 * @return The day component of the DateTime instance.
		 */
		[[nodiscard]] int Day() const { return m_TimeInfo.tm_mday; }

		/**
		 * @brief Returns the month component of the DateTime instance.
		 * @return The month component of the DateTime instance.
		 */
		[[nodiscard]] int Month() const { return m_TimeInfo.tm_mon + 1; }

		/**
		 * @brief Returns the year component of the DateTime instance.
		 * @return The year component of the DateTime instance.
		 */
		[[nodiscard]] int Year() const { return m_TimeInfo.tm_year + 1900; }

		/**
		 * @brief Compares two DateTime instances for equality.
		 * @param rhs The DateTime instance to compare with.
		 * @return True if the two DateTime instances are equal, false otherwise.
		 */
		bool operator==(const DateTime& rhs) const
		{
			return Second() == rhs.Second() && Minute() == rhs.Minute() && Hour() == rhs.Hour() &&
				Day() == rhs.Day() && Month() == rhs.Month() && Year() == rhs.Year();
		}

		/**
		 * @brief Compares two DateTime instances for inequality.
		 * @param rhs The DateTime instance to compare with.
		 * @return True if the two DateTime instances are not equal, false otherwise.
		 */
		bool operator!=(const DateTime& rhs) const { return !(*this == rhs); }

		/**
		 * @brief Sets the time of day for the DateTime instance.
		 * @param time The time of day to set, represented as a float value.
		 */
		static void SetTimeOfDay(float time);

		/**
		 * @brief Sets the current date and time for the DateTime instance.
		 * @param year The year component of the date to set.
		 * @param month The month component of the date to set.
		 * @param day The day component of the date to set.
		 * @param hour The hour component of the time to set.
		 * @param minute The minute component of the time to set.
		 * @param second The second component of the time to set.
		 */
		static void SetCurrentTime(int year, int month, int day, int hour, int minute, int second);

		/**
		 * @brief Adds a specified number of seconds to the DateTime instance.
		 * @param seconds The number of seconds to add.
		 */
		void AddSeconds(int seconds);

		/**
		 * @brief Adds a specified number of minutes to the DateTime instance.
		 * @param minutes The number of minutes to add.
		 */
		void AddMinutes(int minutes);

		/**
		 * @brief Adds a specified number of hours to the DateTime instance.
		 * @param hours The number of hours to add.
		 */
		void AddHours(int hours);

		/**
		 * @brief Adds a specified number of days to the DateTime instance.
		 * @param days The number of days to add.
		 */
		void AddDays(int days);

		/**
		 * @brief Adds a specified number of months to the DateTime instance.
		 * @param months The number of months to add.
		 */
		void AddMonths(int months);

		/**
		 * @brief Adds a specified number of years to the DateTime instance.
		 * @param years The number of years to add.
		 */
		void AddYears(int years);

		/**
		 * @brief Converts the DateTime instance to a 64-bit integer representation.
		 * @return The 64-bit integer representation of the DateTime instance.
		 */
		[[nodiscard]] uint64_t ToNumber() const;

		/**
		 * @brief Creates a DateTime instance from a 64-bit integer representation.
		 * @param number The 64-bit integer representation of the DateTime instance.
		 * @return A DateTime instance corresponding to the provided 64-bit integer representation.
		 */
		static DateTime FromNumber(uint64_t number);

		/**
		 * @brief Converts the DateTime instance to a string representation.
		 * @return The string representation of the DateTime instance.
		 */
		[[nodiscard]] std::string ToString() const;

		/**
		 * @brief Parses a string representation of a date and time into a DateTime instance.
		 * @param input The string representation of the date and time to parse.
		 * @return A DateTime instance corresponding to the provided string representation.
		 */
		static DateTime Parse(const std::string &input);

		/**
		 * @brief Returns a reference to the singleton instance of the DateTime class.
		 * @return A reference to the singleton instance of the DateTime class.
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

		std::tm m_TimeInfo{}; // Standard C library structure to hold date and time components (year, month, day, hour, minute, second)
	};
}

// -----------------------------------------------------
