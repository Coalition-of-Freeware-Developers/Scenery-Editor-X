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
 * monitor_data.h
 * -------------------------------------------------------
 * Created: 18/5/2025
 * -------------------------------------------------------
 */
#pragma once
#include <SDL3/SDL_video.h>
#include <SceneryEditorX/core/base.h>

// --------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class Monitor
	 * @brief Represents a monitor with its properties and statistics.
	 */
	struct Monitor
	{
		std::string monitorID;  // Monitor identifier.
		Vec2 resolution;		// Resolution in pixels.
		Vec2 dimensions;		// Physical size in inches.
		Vec2 pixDensity;		// Pixel density (points per inch).
		int refreshRate;        // Monitor Refresh Rate.
		bool isPrimary;         // True if primary monitor.
		SDL_DisplayID handle;   // Handle to the SDL3 display.
	};

	/**
	 * @class MonitorData
	 * @brief Manages monitor information and statistics using SDL3
	 */
	class MonitorData
	{
	public:
		/* @brief Constructor for MonitorData class. Initializes monitor data and refreshes the monitor list. */
		MonitorData();

		/* @brief Destructor for MonitorData class. Cleans up any resources owned by the class. */
		~MonitorData();

	    /**
		 * @brief Retrieves the current monitor statistics.
		 * @return A vector of Monitor structures containing the current monitor statistics.
		 */
		std::vector<Monitor> GetMonitorStats();

		/* @brief Presents the current monitor statistics. */
		void PresentMonitorStats() const;

		/**
		 * @brief Retrieves the center position of a monitor.
		 * @param displays Optional array of display handles. If nullptr, uses the current monitor.
		 * @return A Vec2 representing the center position of the monitor.
		 */
		Vec2 GetMonitorCenter(SDL_DisplayID *displays = nullptr);
	
		/**
		 * @brief Retrieves the primary monitor.
		 * @return The SDL_DisplayID of the primary monitor.
		 */
		[[nodiscard]] SDL_DisplayID GetPrimaryMonitor() const;

		/**
		 * @brief Retrieves the current monitor.
		 * @return The SDL_DisplayID of the current monitor.
		 */
		[[nodiscard]] SDL_DisplayID GetCurrentMonitor() const;

		/**
		 * @brief Retrieves the list of all monitors.
		 * @return A constant reference to a vector of Monitor structures.
		 */
		[[nodiscard]] const std::vector<Monitor> &GetMonitors() const { return monitors; }

		/**
		 * @brief Retrieves the total number of monitors detected.
		 * @return The number of monitors.
		 */
		[[nodiscard]] int GetMonitorCount() const { return monitorCount; }

		/**
		 * @brief Retrieves the index of the currently selected monitor.
		 * @return The index of the current monitor.
		 */
		[[nodiscard]] int GetCurrentMonitorIndex() const { return monitorIndex; }

		/* @brief Refreshes the count of available displays. */
		void RefreshDisplayCount();

		/* @brief Refreshes the list of available monitors and their statistics. */
		void RefreshMonitorList();

		/**
		 * @brief Retrieves the available video modes for a specific monitor
		 * @code 
		 * int modeCount;
		 * const SDL_DisplayMode **modes = monitorData.GetVideoModes(0, &modeCount);
		 *	// Use modes...
		 * SDL_free(modes); // Free when done
		 * @endcode
		 * @warning Caller MUST free the returned pointer with SDL_free() when done to prevent memory leaks
		 */
		const SDL_DisplayMode **GetVideoModes(int monitorIndex, int *count);

		/**
		 * @brief Retrieves the index of the currently selected video mode.
		 * @return The index of the current video mode.
		 */
		[[nodiscard]] int GetVideoModeIndex() const { return videoModeIndex; }

		/**
		 * @brief Sets the index of the currently selected video mode.
		 * @param index The index of the video mode to set.
		 */
		void SetVideoModeIndex(int index) { videoModeIndex = index; }

		/**
		 * @brief Retrieves the currently selected video mode.
		 * @return A pointer to the SDL_DisplayMode structure of the current video mode.
		 */
		[[nodiscard]] const SDL_DisplayMode *GetCurrentVideoMode() const;

		/**
		 * @brief Retrieves the width of the current monitor.
		 * @return The width of the current monitor in pixels.
		 */
		static uint32_t GetWidth();

		/**
		 * @brief Retrieves the height of the current monitor.
		 * @return The height of the current monitor in pixels.
		 */
		static uint32_t GetHeight();

		/**
		 * @brief Retrieves the refresh rate of the current monitor.
		 * @return The refresh rate of the current monitor in Hz.
		 */
		static float GetRefreshRate(); // Returns raw float refresh rate (e.g., 59.94 Hz). Monitor struct stores rounded int.

		/**
		 * @brief Retrieves the ID of the current monitor.
		 * @return The ID of the current monitor.
		 */
		static uint32_t GetId();

		/**
		 * @brief Retrieves whether the current monitor supports HDR.
		 * @return True if HDR is supported, false otherwise.
		 */
		static bool GetHdr();

		/**
		 * @brief Retrieves the maximum luminance of the current monitor.
		 * @return The maximum luminance of the current monitor in nits.
		 */
		static float GetLuminanceMax();

		/**
		 * @brief Retrieves the gamma value of the current monitor.
		 * @return The gamma value of the current monitor.
		 */
		static float GetGamma();

		/**
		 * @brief Retrieves the name of the current monitor.
		 * @return The name of the current monitor.
		 */
		static const char *GetName();

	private:
		std::vector<Monitor> monitors;	// List of detected monitors with detailed stats
		SDL_DisplayID *monitorHandles;	// Array of monitor handles (managed by SDL3)
		SDL_DisplayID primaryMonitor;	// Handle to the primary monitor
	
		int monitorCount;   // Total number of monitors detected
		int monitorIndex;   // Index of the currently selected monitor
		int videoModeIndex; // Index of the currently selected video mode for the current monitor
	
		/**
		 * @brief Reserved for future API expansion - currently a no-op
		 * @deprecated This method is reserved for future event-driven monitor updates
		 * @note Use RefreshMonitorList() for full monitor enumeration
		 */
		void UpdateMonitorList();
	};

}

// --------------------------------------------
