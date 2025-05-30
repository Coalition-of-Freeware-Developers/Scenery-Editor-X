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
#include <GLFW/glfw3.h>
#include <SceneryEditorX/utils/monitor_data.h>
#include <fmt/format.h>

/// --------------------------------------------

namespace SceneryEditorX
{
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
     * @return std::vector<MonitorStats> containing detailed statistics for each monitor
     * 
     * @see RefreshMonitors
     * @see MonitorStats
     * @see PresentMonitorStats
     */
	std::vector<MonitorInfo::MonitorStats> MonitorInfo::GetMonitorStats()
	{
	    std::vector<MonitorStats> stats;
        RefreshMonitors();
        
        // Reserve space for monitors to avoid reallocation
        stats.reserve(monitorCount);
        GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();

		for (int i = 0; i < monitorCount; ++i)
		{
            GLFWmonitor* currentMonitor = monitor[i];

			MonitorStats data;
            data.monitorID = glfwGetMonitorName(currentMonitor);
			
			/// Get indexed monitor resolution.
            const GLFWvidmode* mode = glfwGetVideoMode(currentMonitor);
            data.resolution = Vec2(mode->width, mode->height);
            data.refreshRate = mode->refreshRate;

			/// Get indexed monitor physical size.
			/// @note Monitor size is in millimeters. Convert to inches.
            int width, height;
            glfwGetMonitorPhysicalSize(currentMonitor, &width, &height);
            data.dimensions = Vec2(width, height) / 25.4f;

			// Calculate pixel density more efficiently
            Vec2 vecPixelDensity = data.resolution / data.dimensions;
            data.pixDensity = Vec2((vecPixelDensity.x + vecPixelDensity.y) * 0.5f);
            data.isPrimary = (currentMonitor == primaryMonitor);
            stats.push_back(std::move(data));
        }

	    return stats;
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
     * @note This method automatically calls GetMonitorStats() to refresh monitor data
     *       before displaying the information
     * 
     * @see GetMonitorStats
     * @see MonitorStats
     */
	void MonitorInfo::PresentMonitorStats()
    {
        for (const auto& data : GetMonitorStats())
        {
            constexpr auto formatString = "{}{}: {} x {} pixels, {:0.1f} x {:0.1f} inches, {:0.2f} Pixels\n";
            fmt::print(fmt::runtime(formatString),
                       data.monitorID,
                       data.isPrimary ? " (Primary)" : "",
                       data.resolution.x,
                       data.resolution.y,
                       data.dimensions.x,
                       data.dimensions.y,
                       data.pixDensity);
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
     *                 will fall back to using the primary monitor.
     * @return Vec2 The center coordinates of the monitor (x, y) in pixels
     * 
     * @note The method contains recursive behavior when invalid monitor data is provided
     * @note This method depends on the current monitorIndex and monitorCount class members
     */
    Vec2 MonitorInfo::GetMonitorCenter(GLFWmonitor **monitors)
    {
        /// Check if monitors are available and monitorIndex is valid
		if (monitorIndex < 0 || monitorIndex >= monitorCount || monitors == nullptr)
		{
            // Get primary monitor and reset monitor index
            RefreshMonitors();
            monitorIndex = 0;
            return GetMonitorCenter(monitor);  // Recursive call with valid monitor
		}

        const GLFWvidmode *mode = glfwGetVideoMode(monitors[monitorIndex]);
        if (!mode) {
            // Fallback if mode cannot be retrieved
            return {0, 0};
        }

		const int screenCenterX = mode->width / 2;
		const int screenCenterY = mode->height / 2;  // Fixed bug: was using width instead of height

		return {screenCenterX, screenCenterY};
    }

    /**
     * @brief Updates the list of available monitors
     * 
     * This method retrieves the current list of monitors connected to the system
     * using the GLFW API and updates the internal monitor count. If the current
     * monitor index is out of range after the refresh (e.g., a monitor was
     * disconnected), it will reset the monitor index to 0 (the primary monitor).
     * 
     * @note This method should be called whenever monitor configuration changes
     *       or before accessing monitor-related information to ensure data is current.
     * 
     * @see GetMonitorStats
     * @see GetMonitorCenter
     */
    void MonitorInfo::RefreshMonitors()
    {
        monitor = glfwGetMonitors(&monitorCount);
        if (monitorIndex >= monitorCount)
            monitorIndex = 0;
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
     * @note The returned array is allocated and owned by GLFW, so it should not be freed
     * @note If an invalid monitor index is provided, count will be set to 0 and nullptr returned
     * 
     * @see RefreshMonitors
     */
    const GLFWvidmode* MonitorInfo::GetVideoModes(const int monitorIndex, int* count)
    {
        if (monitorIndex < 0 || monitorIndex >= monitorCount)
        {
            *count = 0;
            return nullptr;  // Return nullptr instead of calling RefreshMonitors
        }
        
        return glfwGetVideoModes(monitor[monitorIndex], count);
    }

} // namespace SceneryEditorX

/// --------------------------------------------

