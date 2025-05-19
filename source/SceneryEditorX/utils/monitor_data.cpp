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

	std::vector<MonitorInfo::MonitorStats> MonitorInfo::GetMonitorStats()
	{
	    std::vector<MonitorStats> stats;
        RefreshMonitors();

		for (int i = 0; i < monitorCount; ++i)
		{
            auto monitor = monitors[i];

			MonitorStats data;
            data.monitorID = glfwGetMonitorName(monitor);

			/**
			 * @brief Get indexed monitor resolution.
			 */
            data.resolution = Vec2(0);
            const GLFWvidmode *mode = glfwGetVideoMode(monitor);
            data.resolution = Vec2(mode->width, mode->height);
            data.refreshRate = mode->refreshRate;

			/**
			 * @brief Get indexed monitor physical size.
			 *
			 * @note Monitor size is in millimeters. Convert to inches.
			 */
            int width, height;
            glfwGetMonitorPhysicalSize(monitor, &width, &height);
            data.dimensions = Vec2(width, height) / 25.4f;

			auto vecPixelDensity = Vec2(data.resolution) / data.dimensions;
            data.pixDensity = Vec2(vecPixelDensity.x + vecPixelDensity.y) * 0.5f;
            data.isPrimary = monitor == glfwGetPrimaryMonitor();
            stats.push_back(data);
        }

	    return stats;
	}

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
    
    void MonitorInfo::RefreshMonitors()
    {
        monitors = glfwGetMonitors(&monitorCount);
        if (monitorIndex >= monitorCount)
        {
            monitorIndex = 0;
        }
    }
    
    const GLFWvidmode* MonitorInfo::GetVideoModes(int monitorIndex, int* count)
    {
        if (monitorIndex < 0 || monitorIndex >= monitorCount)
        {
            *count = 0;
            return nullptr;
        }
        
        return glfwGetVideoModes(monitors[monitorIndex], count);
    }

} // namespace SceneryEditorX

/// --------------------------------------------

