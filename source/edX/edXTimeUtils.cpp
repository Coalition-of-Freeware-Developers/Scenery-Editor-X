/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXTimeUtils.cpp
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#include "edXTimeUtils.h"
#include <iomanip>
#include <sstream>

/// ----------------------------------------------------------------------------

namespace edx
{
    std::string time_point_to_iso_string(const std::chrono::system_clock::time_point& tp)
    {
        const auto time_t = std::chrono::system_clock::to_time_t(tp);
        std::stringstream ss;
        ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");
        return ss.str();
    }

    std::chrono::system_clock::time_point iso_string_to_time_point(const std::string& iso_string)
    {
        std::tm tm = {};
        std::stringstream ss(iso_string);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
        return std::chrono::system_clock::from_time_t(std::mktime(&tm));
    }

} // namespace edx

/// ----------------------------------------------------------------------------
