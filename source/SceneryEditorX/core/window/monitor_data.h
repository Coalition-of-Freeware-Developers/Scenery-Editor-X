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
#include "SceneryEditorX/core/base.hpp"

/// --------------------------------------------

namespace SceneryEditorX
{
	struct Monitor
	{
	    std::string monitorID; ///< Monitor identifier.
	    Vec2 resolution;       ///< Resolution in pixels.
	    Vec2 dimensions;       ///< Physical size in inches.
	    Vec2 pixDensity;       ///< Pixel density (points per inch).
	    int refreshRate;       ///< Monitor Refresh Rate.
	    bool isPrimary;        ///< True if primary monitor.
        GLFWmonitor* handle;   ///< Handle to the GLFW monitor.
	};

	class MonitorData
	{
	public:
        MonitorData();
        ~MonitorData();

		/// Monitor data collection methods
        std::vector<Monitor> GetMonitorStats();
        void PresentMonitorStats() const;
        Vec2 GetMonitorCenter(GLFWmonitor **monitors = nullptr);

		/// Monitor access methods
        [[nodiscard]] GLFWmonitor* GetPrimaryMonitor() const;
        [[nodiscard]] GLFWmonitor* GetCurrentMonitor() const;
        [[nodiscard]] const std::vector<Monitor>& GetMonitors() const { return monitors; }
		[[nodiscard]] int GetMonitorCount() const { return monitorCount; }
		[[nodiscard]] int GetCurrentMonitorIndex() const { return monitorIndex; }
		//void SetCurrentMonitorIndex(int index);
		void RefreshDisplayCount();
        void RefreshMonitorList();

		/// Video mode methods
		const GLFWvidmode* GetVideoModes(int monitorIndex, int* count);
		[[nodiscard]] int GetVideoModeIndex() const { return videoModeIndex; }
		void SetVideoModeIndex(int index) { videoModeIndex = index; }
        [[nodiscard]] const GLFWvidmode *GetCurrentVideoMode() const;

	private:
        /// Core monitor data
        std::vector<Monitor> monitors;
        GLFWmonitor** monitorHandles;
        GLFWmonitor* primaryMonitor;

        /// State tracking
        int monitorCount;
        int monitorIndex;
        int videoModeIndex;

        /// Updates the internal monitor list based on the current GLFW monitors
        void UpdateMonitorList();
	};

} // namespace SceneryEditorX

/// --------------------------------------------
