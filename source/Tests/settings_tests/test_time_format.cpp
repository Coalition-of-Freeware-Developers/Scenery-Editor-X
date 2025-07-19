/**
 * Test file to verify DD/MM/YYYY time format conversion
 */

#include <iostream>
#include <ctime>
#include <string>
#include <sstream>
#include <iomanip>

// Simplified versions of the functions for testing
std::string TimeToString(time_t time, bool useSystemFormat = true)
{
    if (time == 0) {
        return "";
    }

    std::tm* timeInfo = std::localtime(&time);
    if (!timeInfo) {
        return "";
    }

    std::stringstream ss;
    if (useSystemFormat) {
        // For testing, we'll use DD/MM/YYYY as our "system" format
        ss << std::put_time(timeInfo, "%d/%m/%Y %H:%M:%S");
    } else {
        // Use our standard DD/MM/YYYY HH:MM:SS format
        ss << std::put_time(timeInfo, "%d/%m/%Y %H:%M:%S");
    }

    return ss.str();
}

time_t StringToTime(const std::string& timeString)
{
    if (timeString.empty()) {
        return 0;
    }

    std::tm tm = {};
    std::istringstream ss(timeString);

    // Try multiple date formats
    std::vector<std::string> formats = {
        "%d/%m/%Y %H:%M:%S",  // DD/MM/YYYY HH:MM:SS (our standard format)
        "%d:%m:%Y %H:%M:%S",  // DD:MM:YYYY HH:MM:SS (old format for backward compatibility)
        "%Y-%m-%d %H:%M:%S",  // ISO format YYYY-MM-DD
        "%m/%d/%Y %H:%M:%S",  // US format MM/DD/YYYY
        "%d-%m-%Y %H:%M:%S",  // DD-MM-YYYY with dashes
    };

    for (const auto& format : formats) {
        ss.clear();
        ss.str(timeString);
        ss >> std::get_time(&tm, format.c_str());

        if (!ss.fail()) {
            time_t result = std::mktime(&tm);
            if (result != -1) {
                std::cout << "Parsed time string '" << timeString
                         << "' with format '" << format << "' -> " << result << std::endl;
                return result;
            }
        }
    }

    std::cout << "Failed to parse time string: '" << timeString << "'" << std::endl;
    return 0;
}

int main()
{
    std::cout << "=== Time Format Conversion Test ===" << std::endl;

    // Get current time
    time_t currentTime = std::time(nullptr);
    std::cout << "Current time_t: " << currentTime << std::endl;

    // Convert to string using DD/MM/YYYY format
    std::string timeStr = TimeToString(currentTime, false);
    std::cout << "DD/MM/YYYY format: " << timeStr << std::endl;

    // Convert back to time_t
    time_t convertedBack = StringToTime(timeStr);
    std::cout << "Converted back: " << convertedBack << std::endl;

    // Test accuracy
    std::cout << "Difference: " << abs(static_cast<long>(currentTime - convertedBack)) << " seconds" << std::endl;

    std::cout << "\n=== Backward Compatibility Test ===" << std::endl;

    // Test old DD:MM:YYYY format
    std::string oldFormat = "11:07:2025 14:30:45";
    std::cout << "Testing old format: " << oldFormat << std::endl;
    time_t oldTime = StringToTime(oldFormat);
    std::cout << "Parsed old format: " << oldTime << std::endl;
    if (oldTime > 0) {
        std::cout << "Converted to new format: " << TimeToString(oldTime, false) << std::endl;
    }

    std::cout << "\n=== New Format Test ===" << std::endl;

    // Test new DD/MM/YYYY format
    std::string newFormat = "11/07/2025 14:30:45";
    std::cout << "Testing new format: " << newFormat << std::endl;
    time_t newTime = StringToTime(newFormat);
    std::cout << "Parsed new format: " << newTime << std::endl;
    if (newTime > 0) {
        std::cout << "Round-trip test: " << TimeToString(newTime, false) << std::endl;
    }

    return 0;
}
