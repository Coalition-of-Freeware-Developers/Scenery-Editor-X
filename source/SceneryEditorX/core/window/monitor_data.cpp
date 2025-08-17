/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* monitor_data.cpp
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#include "monitor_data.h"
#include <algorithm>
#include <GLFW/glfw3.h>
#include <fmt/format.h>
#include "SceneryEditorX/logging/logging.hpp"

/// --------------------------------------------

namespace SceneryEditorX
{
    /**
     * @brief Constructor for MonitorData
     *
     * Initializes the monitor data and refreshes the monitor list
     */
    MonitorData::MonitorData() : monitorHandles(nullptr), primaryMonitor(nullptr), monitorCount(0), monitorIndex(0), videoModeIndex(0)
    {
        try
		{
            RefreshDisplayCount();
            RefreshMonitorList();
        }
        catch (const std::exception& e)
		{
            SEDX_CORE_WARN("Exception during MonitorData initialization: {}", e.what()); /// Continue with default values rather than crashing
        }
    }

    /**
     * @brief Destructor for MonitorData
     *
     * Cleans up any resources owned by the class
     * @note: GLFW manages the lifetime of monitor handles, so we don't need to free them
     */
    MonitorData::~MonitorData() = default;


    /**
     * @brief Retrieves detailed statistics for all connected monitors
     *
     * This method collects comprehensive information about each connected display monitor,
     * including:
     * - Monitor identifier/name
     * - Resolution in pixels (width × height)
     * - Physical dimensions in inches (converted from mm)
     * - Pixel density (PPI - pixels per inch)
     * - Refresh rate in Hz
     * - Primary monitor status
     *
     * The method automatically refreshes the monitor list before collecting data to ensure
     * that the most current configuration is used, handling monitor connection/disconnection
     * events appropriately.
     *
     * @return std::vector<Monitor> containing detailed statistics for each monitor
     *
     * @see RefreshDisplayCount
     * @see RefreshMonitorList
     * @see PresentMonitorStats
     */
	std::vector<Monitor> MonitorData::GetMonitorStats()
	{
	    try
		{
	        /// Update monitor data if needed
            RefreshDisplayCount();

            /// If we already have monitors, return them
            if (!monitors.empty())
                return monitors;

            /// Otherwise, populate the monitor data
            RefreshMonitorList();
            for (const auto &displayName : monitors)
                SEDX_CORE_INFO("Monitor: {} = {}", monitorCount, displayName.monitorID);

            return monitors;

        }
        catch (const std::exception& e)
		{
            SEDX_CORE_WARN("Exception in GetMonitorStats: {}", e.what());
            return {}; /// Return empty vector on error
        }
	}

    /**
     * @brief Displays detailed statistics for all connected monitors to the console
     *
     * This method prints formatted information about each connected monitor to the
     * standard output, including:
     * - Monitor identifier/name with primary monitor indication
     * - Resolution in pixels (width × height)
     * - Physical dimensions in inches
     * - Pixel density (PPI - pixels per inch)
     *
     * The output is formatted using fmt library with consistent spacing and
     * appropriate precision for floating-point values.
     *
     * @note - This method automatically calls GetMonitorStats() to refresh monitor data
     *       before displaying the information
     *
     * @see GetMonitorStats
     * @see Monitor
     */
	void MonitorData::PresentMonitorStats() const
    {
        try
		{
            for (const auto& data : GetMonitors())
            {
                SEDX_CORE_TRACE("{}{}: {} x {} pixels, {:0.1f} x {:0.1f} inches, {:0.2f} Pixels",
                           data.monitorID,
                           data.isPrimary ? " (Primary)" : "",
                           data.resolution.x,
                           data.resolution.y,
                           data.dimensions.x,
                           data.dimensions.y,
                           data.pixDensity.x);
            }
        }
        catch (const std::exception& e)
		{
            SEDX_CORE_WARN("Exception in PresentMonitorStats: {}", e.what());
        }
    }

    /**
     * @brief Calculates the center point coordinates of the current monitor
     *
     * This method determines the center point of the specified monitor. If no valid
     * monitor is available or the monitor index is out of range, it will automatically
     * switch to the primary monitor and recursively call itself to get the center point.
     *
     * @param monitors Pointer to an array of GLFW monitor pointers. If nullptr, method
     *                 will use the current monitor handles.
     * @return Vec2 The center coordinates of the monitor (x, y) in pixels
     *
     * @note - The method contains recursive behavior when invalid monitor data is provided
     * @note - This method depends on the current monitorIndex and monitorCount class members
     */
    Vec2 MonitorData::GetMonitorCenter(GLFWmonitor **monitors)
    {
        try
		{
            GLFWmonitor **monitorsToUse = (monitors != nullptr) ? monitors : monitorHandles;

            /// Check if monitors are available and monitorIndex is valid
    		if (monitorIndex < 0 || monitorIndex >= monitorCount || monitorsToUse == nullptr)
    		{
                /// Get primary monitor and reset monitor index
                RefreshDisplayCount();
                monitorIndex = 0;

                /// Safety check to prevent infinite recursion
                if (monitorHandles && monitorCount > 0)
                    return GetMonitorCenter(monitorHandles);  /// Recursive call with valid monitor

                SEDX_CORE_WARN("No valid monitors available for GetMonitorCenter");
                return {640.0f, 360.0f};  /// Default fallback center
            }

            const GLFWvidmode *mode = glfwGetVideoMode(monitorsToUse[monitorIndex]);
            if (!mode)
			{
                /// Fallback if mode cannot be retrieved
                SEDX_CORE_WARN("Failed to get video mode for monitor {}", monitorIndex);
                return {640.0f, 360.0f};  /// Default fallback center
            }

    		const int screenCenterX = mode->width / 2;
    		const int screenCenterY = mode->height / 2;

    		return {static_cast<float>(screenCenterX), static_cast<float>(screenCenterY)};
        }
        catch (const std::exception& e)
		{
            SEDX_CORE_WARN("Exception in GetMonitorCenter: {}", e.what());
            return {640.0f, 360.0f};  /// Default fallback center
        }
    }

    /**
     * @brief Updates the list of available monitors
     *
     * This method retrieves the current list of monitors connected to the system
     * using the GLFW API and updates the internal monitor count. If the current
     * monitor index is out of range after the refresh (e.g., a monitor was
     * disconnected), it will reset the monitor index to 0 (the primary monitor).
     *
     * @note - This method should be called whenever monitor configuration changes
     *       or before accessing monitor-related information to ensure data is current.
     * @note - Requires GLFW to be initialized before calling this method.
     *
     * @see GetMonitorStats
     * @see GetMonitorCenter
     */
    void MonitorData::RefreshDisplayCount()
    {
        /// Check if GLFW is initialized
        if (int initialized = glfwInit(); !initialized)
		{
            SEDX_CORE_WARN("Cannot refresh display count - GLFW not initialized");
            monitorCount = 0;
            monitorHandles = nullptr;
            primaryMonitor = nullptr;
            return;
        }

        /// Get monitors from GLFW
        monitorHandles = glfwGetMonitors(&monitorCount);
        primaryMonitor = glfwGetPrimaryMonitor();

        /// Safety check to ensure we have at least one monitor
        if (monitorCount <= 0)
		{
            SEDX_CORE_WARN("No monitors detected during RefreshDisplayCount");
            monitorCount = 0;
            monitorHandles = nullptr;
            primaryMonitor = nullptr;
        }
        else
            SEDX_CORE_INFO("Detected {} monitor(s)", monitorCount);

        /// Make sure the current monitor index is valid
        if (monitorIndex >= monitorCount)
            monitorIndex = 0;
    }

    void MonitorData::RefreshMonitorList()
    {
        if (glfwInit() == GLFW_FALSE)
        {
            SEDX_CORE_WARN("GLFW not initialized - cannot refresh monitor list");
            return;
        }

        int count;
        monitorHandles = glfwGetMonitors(&count);

        monitorCount = count;

        if (monitorCount > 0)
        {
            SEDX_CORE_INFO("Successfully refreshed monitor list: {} monitor(s) detected", monitorCount);
            primaryMonitor = glfwGetPrimaryMonitor();
        }
        else
        {
            SEDX_CORE_WARN("No monitors detected");
            primaryMonitor = nullptr;
        }
    }

    /**
     * @brief Retrieves the available video modes for a specific monitor
     *
     * This method returns all video modes supported by the specified monitor.
     * Each video mode contains resolution, color depth, and refresh rate information.
     *
     * @param monitorIndex Index of the monitor to query (must be in range [0, monitorCount-1])
     * @param count Pointer to an integer where the number of available video modes will be stored
     * @return const GLFWvidmode* Array of video modes or nullptr if the monitor index is invalid
     *
     * @note - The returned array is allocated and owned by GLFW, so it should not be freed
     * @note - If an invalid monitor index is provided, count will be set to 0 and nullptr returned
     *
     * @see RefreshMonitorList
     */
    const GLFWvidmode* MonitorData::GetVideoModes(const int monitorIndex, int* count)
    {
        /// Set default return value for error cases
        *count = 0;

        try
		{
            /// Validate index and monitor handles
            if (monitorIndex < 0 || monitorIndex >= monitorCount || monitorHandles == nullptr)
			{
                SEDX_CORE_WARN("Invalid monitor index {} for GetVideoModes (total: {})", monitorIndex, monitorCount);
                return nullptr;
            }

            /// Get video modes from GLFW
            const GLFWvidmode* modes = glfwGetVideoModes(monitorHandles[monitorIndex], count);

            /// Log result
            if (modes && *count > 0)
                SEDX_CORE_INFO("Retrieved {} video modes for monitor {}", *count, monitorIndex);
            else
			{
                SEDX_CORE_WARN("No video modes available for monitor {}", monitorIndex);
                *count = 0;
            }

            return modes;
        }
        catch (const std::exception& e)
		{
            SEDX_CORE_WARN("Exception in GetVideoModes: {}", e.what());
            return nullptr;
        }
    }

    /**
     * @brief Gets the primary monitor
     *
     * @return GLFWmonitor* Handle to the primary monitor or nullptr if none available
     */
    GLFWmonitor* MonitorData::GetPrimaryMonitor() const
    {
        if (!primaryMonitor)
            SEDX_CORE_WARN("Primary monitor not available");

        return primaryMonitor;
    }

    /**
     * @brief Gets the currently selected monitor
     *
     * @return GLFWmonitor* Handle to the current monitor or nullptr if none available
     */
    GLFWmonitor* MonitorData::GetCurrentMonitor() const
    {
        if (monitorIndex >= 0 && monitorIndex < monitorCount && monitorHandles != nullptr)
            return monitorHandles[monitorIndex];

        SEDX_CORE_WARN("Current monitor not available (index: {}, count: {})", monitorIndex, monitorCount);
        return nullptr;
    }

    /**
     * @brief Gets the current video mode of the selected monitor
     *
     * @return const GLFWvidmode* Current video mode or nullptr if not available
     */
    const GLFWvidmode* MonitorData::GetCurrentVideoMode() const
    {
        if (GLFWmonitor* monitor = GetCurrentMonitor())
		{
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (!mode)
                SEDX_CORE_WARN("Failed to get video mode for current monitor");

            return mode;
        }

        SEDX_CORE_WARN("Cannot get current video mode - no monitor available");
        return nullptr;
    }

} // namespace SceneryEditorX

/// --------------------------------------------

