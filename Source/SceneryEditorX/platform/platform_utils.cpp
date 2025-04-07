/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* platform_utils.cpp
* -------------------------------------------------------
* Created: 5/4/2025
* -------------------------------------------------------
*/
#include <chrono>
#include <ctime>
#include <SceneryEditorX/platform/platform_utils.h>
#include "spdlog/fmt/chrono.h"

// -------------------------------------------------------

namespace SceneryEditorX::IO
{
	
	uint64_t Platform::GetCurrentDateTimeU64()
	{
		std::string string = GetCurrentDateTimeString();
		return std::stoull(string);
	}

	std::string Platform::GetCurrentDateTimeString()
	{
		std::time_t currentTime = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		std::tm* localTime = std::localtime(&currentTime);

		int year = localTime->tm_year + 1900;
		int month = localTime->tm_mon + 1;
		int day = localTime->tm_mday + 1;
		int hour = localTime->tm_hour;
		int minute = localTime->tm_min;

		return std::format("{}{:02}{:02}{:02}{:02}", year, month, day, hour, minute);
		//return std::format("{:%Y%m%d%H%M}", *localTime);
	}

} // namespace SceneryEditorX::IO

// -------------------------------------------------------
