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
	
	class MonitorData
	{
	public:
	    MonitorData();
	    ~MonitorData();
	
	    // Monitor data collection methods
	    std::vector<Monitor> GetMonitorStats();
	    void PresentMonitorStats() const;
	    Vec2 GetMonitorCenter(SDL_DisplayID *displays = nullptr);
	
	    // Monitor access methods
	    [[nodiscard]] SDL_DisplayID GetPrimaryMonitor() const;
	    [[nodiscard]] SDL_DisplayID GetCurrentMonitor() const;
	    [[nodiscard]] const std::vector<Monitor> &GetMonitors() const { return monitors; }
	    [[nodiscard]] int GetMonitorCount() const { return monitorCount; }
	    [[nodiscard]] int GetCurrentMonitorIndex() const { return monitorIndex; }
	    void RefreshDisplayCount();
	    void RefreshMonitorList();

	    /** @warning IMPORTANT: Caller MUST free the returned pointer with SDL_free() when done */
	    const SDL_DisplayMode **GetVideoModes(int monitorIndex, int *count);
	    [[nodiscard]] int GetVideoModeIndex() const { return videoModeIndex; }
	    void SetVideoModeIndex(int index) { videoModeIndex = index; }
	    [[nodiscard]] const SDL_DisplayMode *GetCurrentVideoMode() const;
	
	    // Static convenience methods for current monitor
	    static uint32_t GetWidth();
	    static uint32_t GetHeight();
	    static float
	    GetRefreshRate(); // Returns raw float refresh rate (e.g., 59.94 Hz). Monitor struct stores rounded int.
	    static uint32_t GetId();
	    static bool GetHdr();
	    static float GetLuminanceMax();
	    static float GetGamma();
	    static const char *GetName();
	
	private:
	    // Core monitor data
	    std::vector<Monitor> monitors;
	    SDL_DisplayID *monitorHandles;
	    SDL_DisplayID primaryMonitor;
	
	    // State tracking
	    int monitorCount;
	    int monitorIndex;
	    int videoModeIndex;
	
	    // @brief Reserved for future API expansion - currently a no-op
	    // @deprecated This method is reserved for future event-driven monitor updates
	    // @note Use RefreshMonitorList() for full monitor enumeration
	    void UpdateMonitorList();
	};

}

// --------------------------------------------
