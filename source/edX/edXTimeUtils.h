/**
* -------------------------------------------------------
* Scenery Editor X - edX File Format
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* edXTimeUtils.h
* -------------------------------------------------------
* Created: 27/5/2025
* -------------------------------------------------------
*/
#pragma once
#include "edXConfig.h"
#include <chrono>
#include <string>

/// ----------------------------------------------------------------------------

namespace edx
{
    /**
     * @brief Converts a time_point to ISO 8601 string format
     * 
     * Converts a std::chrono::system_clock::time_point to a formatted ISO 8601 
     * string in UTC timezone (format: YYYY-MM-DDTHH:MM:SSZ).
     * 
     * @param tp The time_point to convert
     * @return std::string The formatted ISO 8601 string
     */
    EDX_API std::string time_point_to_iso_string(const std::chrono::system_clock::time_point& tp);

    /**
     * @brief Converts an ISO 8601 string to a time_point
     * 
     * Parses an ISO 8601 formatted string and converts it to a 
     * std::chrono::system_clock::time_point object.
     * 
     * @param iso_string The ISO 8601 formatted string to parse
     * @return std::chrono::system_clock::time_point The parsed time_point
     */
    EDX_API std::chrono::system_clock::time_point iso_string_to_time_point(const std::string& iso_string);

} // namespace edx

/// ----------------------------------------------------------------------------
