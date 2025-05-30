/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* monitor_data.h
* -------------------------------------------------------
* Created: 18/5/2025
* -------------------------------------------------------
*/
#pragma once
#include <GLFW/glfw3.h>
#include <string>
#include <vector>

/// --------------------------------------------

namespace SceneryEditorX
{
	class MonitorInfo
	{
	public:
		struct MonitorStats
		{
            std::string monitorID; ///< Monitor identifier.
            Vec2 resolution;       ///< Resolution in pixels.
            Vec2 dimensions;       ///< Physical size in inches.
            Vec2 pixDensity;       ///< Pixel density (points per inch).
            int refreshRate;       ///< Monitor Refresh Rate.
            bool isPrimary;        ///< True if primary monitor.
		};

		/// Static methods for monitor management
		GLOBAL std::vector<MonitorStats> GetMonitorStats();
		GLOBAL void PresentMonitorStats();
        GLOBAL Vec2 GetMonitorCenter(GLFWmonitor **monitors);
		
		/// Monitor access methods
		GLOBAL GLFWmonitor** GetPriMonitor() { return monitor; }
		GLOBAL int GetMonitorCount() { return monitorCount; }
		GLOBAL int GetCurrentMonitorIndex() { return monitorIndex; }
		GLOBAL void SetCurrentMonitorIndex(int index) { monitorIndex = index; }
		GLOBAL void RefreshMonitors();
		
		/// Video mode methods
		GLOBAL const GLFWvidmode* GetVideoModes(int monitorIndex, int* count);
		GLOBAL int GetVideoModeIndex() { return videoModeIndex; }
		GLOBAL void SetVideoModeIndex(int index) { videoModeIndex = index; }
		
	private:
		INTERNAL inline GLFWmonitor** monitor = nullptr;
		INTERNAL inline int monitorCount = 0;
		INTERNAL inline int monitorIndex = 0;
		INTERNAL inline int videoModeIndex = 0;
	};

} // namespace SceneryEditorX

/// --------------------------------------------
