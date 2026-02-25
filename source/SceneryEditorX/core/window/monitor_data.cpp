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
 * monitor_data.cpp
 * -------------------------------------------------------
 * Created: 18/5/2025
 * -------------------------------------------------------
 */
#include "monitor_data.h"
#include "window.h"
#include <algorithm>
#include <SDL3/SDL_video.h>
#include <SceneryEditorX/logging/logging.hpp>
#include <fmt/format.h>
#include <tracy/Tracy.hpp>

// --------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @brief Constructor for MonitorData
	 * Initializes the monitor data and refreshes the monitor list
	 */
    MonitorData::MonitorData() : monitorHandles(nullptr), primaryMonitor(0), monitorCount(0), monitorIndex(0), videoModeIndex(0)
	{
	    try
	    {
	        RefreshDisplayCount();
	        RefreshMonitorList();
	    }
	    catch (const std::exception &e)
	    {
	        SEDX_CORE_WARN("Exception during MonitorData initialization: {}", e.what()); // Continue with default values rather than crashing
	    }
	}
	
	/**
	 * @brief Destructor for MonitorData
	 *
	 * Cleans up any resources owned by the class
	 * @note: SDL3 display handles need to be freed with SDL_free
	 */
	MonitorData::~MonitorData()
	{
	    if (monitorHandles)
	    {
	        SDL_free(monitorHandles);
	        monitorHandles = nullptr;
	    }
	}
	
	
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
	        // Tick monitor data if needed
	        RefreshDisplayCount();
	
	        // If we already have monitors, return them
	        if (!monitors.empty())
	        {
	            return monitors;
	        }
	
	        // Otherwise, populate the monitor data
	        RefreshMonitorList();
	
	        // Clear the existing monitor list
	        monitors.clear();
	
	        // Iterate through all displays and populate monitor data
	        for (int i = 0; i < monitorCount; ++i)
	        {
				Monitor mon{
                    .monitorID = "",						// Monitor/display name 
                    .resolution = Vec2(0.0f, 0.0f),    // resolution
                    .dimensions = Vec2(0.0f, 0.0f),    // physical dimensions in inches
                    .pixDensity = Vec2(96.0f, 96.0f),  // pixel density (PPI)
                    .refreshRate = 0,						// refresh rate in Hz
                    .isPrimary = false,						// Is the primary monitor
                    .handle = 0								// Handle to the monitor
				};

	            SDL_DisplayID displayID = monitorHandles[i];
	
	            // Get display name
	            const char *name = SDL_GetDisplayName(displayID);
	            mon.monitorID = std::string(name ? name : "Unknown");
	
	            // Get current display mode
	            if (const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(displayID))
	            {
	                mon.resolution = Vec2(static_cast<float>(mode->w), static_cast<float>(mode->h));
	                mon.refreshRate = static_cast<int>(std::round(mode->refresh_rate));
	            }
	            else
	            {
	                mon.resolution = Vec2(0.0f, 0.0f);
	                mon.refreshRate = 0;
	            }
	
	            // Get physical dimensions using display bounds
	            SDL_Rect bounds;
	            if (SDL_GetDisplayBounds(displayID, &bounds))
	            {
	                // Get DPI information to calculate physical size
	                float contentScale = SDL_GetDisplayContentScale(displayID);
	
	                // Check for valid content scale, use default if invalid
	                if (contentScale <= 0.0f)
	                {
	                    contentScale = 1.0f;
	                }
	
	                float horizontalDPI = 96.0f * contentScale;
	                float verticalDPI	= horizontalDPI;
	
	                // Calculate physical dimensions in inches
	                if (horizontalDPI > 0.0f && verticalDPI > 0.0f)
	                {
	                    mon.dimensions.x = mon.resolution.x / horizontalDPI;
	                    mon.dimensions.y = mon.resolution.y / verticalDPI;
	                    mon.pixDensity.x = horizontalDPI;
	                    mon.pixDensity.y = verticalDPI;
	                }
	                else
	                {
	                    mon.dimensions = Vec2(0.0f, 0.0f);
	                    mon.pixDensity = Vec2(96.0f, 96.0f);
	                }
	            }
	            else
	            {
	                mon.dimensions = Vec2(0.0f, 0.0f);
	                mon.pixDensity = Vec2(96.0f, 96.0f);
	            }
	
	            // Check if this is the primary monitor
	            mon.isPrimary = (displayID == primaryMonitor);
	            mon.handle = displayID;
	
	            monitors.push_back(mon);
	            SEDX_CORE_TRACE("Monitor {}: {}", i, mon.monitorID);
	        }
	
	        return monitors;
	    }
	    catch (const std::exception &e)
	    {
	        SEDX_CORE_WARN("Exception in GetMonitorStats: {}", e.what());
	        return {}; // Return empty vector on error
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
	        for (const auto &data : GetMonitors())
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
	    catch (const std::exception &e)
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
	 * @param displays Pointer to an array of SDL display IDs. If nullptr, method
	 *                 will use the current monitor handles.
	 * @return Vec2 The center coordinates of the monitor (x, y) in pixels
	 *
	 * @note - The method contains recursive behavior when invalid monitor data is provided
	 * @note - This method depends on the current monitorIndex and monitorCount class members
	 */
	Vec2 MonitorData::GetMonitorCenter(SDL_DisplayID *displays)
	{
	    try
	    {
	        SDL_DisplayID *displaysToUse = (displays != nullptr) ? displays : monitorHandles;
	
	        // Check if monitors are available and monitorIndex is valid
	        if (monitorIndex < 0 || monitorIndex >= monitorCount || displaysToUse == nullptr)
	        {
	            // Get primary monitor and reset monitor index
	            RefreshDisplayCount();
	            monitorIndex = 0;
	
	            // Safety check to prevent infinite recursion
	            if (monitorHandles && monitorCount > 0)
	            {
	                return GetMonitorCenter(monitorHandles); // Recursive call with valid monitor
	            }
	
	            SEDX_CORE_WARN("No valid monitors available for GetMonitorCenter");
	            return {640.0f, 360.0f}; // Default fallback center
	        }
	
	        const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(displaysToUse[monitorIndex]);
	        if (!mode)
	        {
	            // Fallback if mode cannot be retrieved
                SEDX_CORE_WARN("Failed to get video mode for monitor {}", monitorIndex);
	            return {640.0f, 360.0f}; // Default fallback center
	        }
	
	        const int screenCenterX = mode->w / 2;
	        const int screenCenterY = mode->h / 2;
	
	        return {static_cast<float>(screenCenterX), static_cast<float>(screenCenterY)};
	    }
	    catch (const std::exception &e)
	    {
	        SEDX_CORE_WARN("Exception in GetMonitorCenter: {}", e.what());
	        return {640.0f, 360.0f}; // Default fallback center
	    }
	}
	
	/**
	 * @brief Gets the primary monitor
	 * @return SDL_DisplayID Handle to the primary display or 0 if none available
	 */
	SDL_DisplayID MonitorData::GetPrimaryMonitor() const
	{
	    if (primaryMonitor == 0)
	    {
	        SEDX_CORE_WARN("Primary monitor not available");
	    }
	
	    return primaryMonitor;
	}
	
	/**
	 * @brief Gets the currently selected monitor
	 *
	 * @return SDL_DisplayID Handle to the current display or 0 if none available
	 */
	SDL_DisplayID MonitorData::GetCurrentMonitor() const
	{
	    if (monitorIndex >= 0 && monitorIndex < monitorCount && monitorHandles != nullptr)
	    {
	        return monitorHandles[monitorIndex];
	    }
	
	    SEDX_CORE_WARN("Current monitor not available (index: {}, count: {})", monitorIndex, monitorCount);
	    return 0;
	}
	
	/**
	 * @brief Updates the list of available monitors
	 *
	 * This method retrieves the current list of monitors connected to the system
	 * using the SDL3 API and updates the internal monitor count. If the current
	 * monitor index is out of range after the refresh (e.g., a monitor was
	 * disconnected), it will reset the monitor index to 0 (the primary monitor).
	 *
	 * @note - This method should be called whenever monitor configuration changes
	 *       or before accessing monitor-related information to ensure data is current.
	 * @note - Requires SDL3 to be initialized before calling this method.
	 *
	 * @see GetMonitorStats
	 * @see GetMonitorCenter
	 */
	void MonitorData::RefreshDisplayCount()
	{
	    // Check if SDL is initialized
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
                SEDX_CORE_WARN("Cannot refresh display count - SDL video initialization failed");
	            monitorCount = 0;
	            monitorHandles = nullptr;
	            primaryMonitor = 0;
	            return;
	        }
	    }
	
	    // Free previous display list if exists
	    if (monitorHandles)
	    {
	        SDL_free(monitorHandles);
	        monitorHandles = nullptr;
	    }
	
	    // Get displays from SDL3
	    monitorHandles = SDL_GetDisplays(&monitorCount);
	    primaryMonitor = SDL_GetPrimaryDisplay();
	
	    // Safety check to ensure we have at least one monitor
	    if (monitorCount <= 0 || !monitorHandles)
	    {
            SEDX_CORE_WARN("No monitors detected during RefreshDisplayCount");
	        monitorCount = 0;
	        if (monitorHandles)
	        {
	            SDL_free(monitorHandles);
	            monitorHandles = nullptr;
	        }
	        primaryMonitor = 0;
	    }
	    else
	    {
            SEDX_CORE_TRACE("Detected {} monitor(s)", monitorCount);
	    }
	
	    // Make sure the current monitor index is valid
	    if (monitorIndex >= monitorCount)
	    {
	        monitorIndex = 0;
	    }
	}
	
	void MonitorData::RefreshMonitorList()
	{
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
                SEDX_CORE_WARN("SDL not initialized - cannot refresh monitor list");
	            return;
	        }
	    }
	
	    // Free previous display list if exists
	    if (monitorHandles)
	    {
	        SDL_free(monitorHandles);
	        monitorHandles = nullptr;
	    }
	
	    monitorHandles = SDL_GetDisplays(&monitorCount);
	
	    if (monitorCount > 0 && monitorHandles)
	    {
            SEDX_CORE_TRACE("Successfully refreshed monitor list: {} monitor(s) detected", monitorCount);
	        primaryMonitor = SDL_GetPrimaryDisplay();
	    }
	    else
	    {
            SEDX_CORE_WARN("No monitors detected");
	        if (monitorHandles)
	        {
	            SDL_free(monitorHandles);
	            monitorHandles = nullptr;
	        }

	        primaryMonitor = 0;
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
	 * @return const SDL_DisplayMode** Array of video modes or nullptr if the monitor index is invalid
	 *
	 * @warning CALLER MUST FREE THE RETURNED ARRAY: The returned pointer is allocated by SDL3
	 *          and MUST be freed by the caller using SDL_free() to prevent memory leaks.
	 *          Example: auto modes = GetVideoModes(0, &count); use modes; SDL_free(modes);
	 * @note - If an invalid monitor index is provided, count will be set to 0 and nullptr returned
	 *
	 * @see RefreshMonitorList
	 */
	const SDL_DisplayMode **MonitorData::GetVideoModes(const int monitorIndex, int *count)
	{
	    // Set default return value for error cases
	    *count = 0;
	
	    try
	    {
	        // Validate index and monitor handles
	        if (monitorIndex < 0 || monitorIndex >= monitorCount || monitorHandles == nullptr)
	        {
                SEDX_CORE_WARN("Invalid monitor index {} for GetVideoModes (total: {})", monitorIndex, monitorCount);
	            return nullptr;
	        }
	
	        // Get video modes from SDL3
	        SDL_DisplayMode **modes = SDL_GetFullscreenDisplayModes(monitorHandles[monitorIndex], count);
	
	        // Log result
	        if (modes && *count > 0)
	        {
	            SEDX_CORE_TRACE("Retrieved {} video modes for monitor {}", *count, monitorIndex);
	        }
	        else
	        {
                SEDX_CORE_WARN("No video modes available for monitor {}", monitorIndex);
	        }
	
	        return const_cast<const SDL_DisplayMode **>(modes);
	    }
	    catch (const std::exception &e)
	    {
            SEDX_CORE_WARN("Exception in GetVideoModes: {}", e.what());
	        return nullptr;
	    }
	}
	
	/**
	 * @brief Gets the current video mode of the selected monitor
	 *
	 * @return const SDL_DisplayMode* Current video mode or nullptr if not available
	 */
	const SDL_DisplayMode *MonitorData::GetCurrentVideoMode() const
	{
        if (SDL_DisplayID display = GetCurrentMonitor(); display != 0)
	    {
	        const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
	        if (!mode)
	        {
	            SEDX_CORE_WARN("Failed to get video mode for current monitor");
	        }
	
	        return mode;
	    }
	
	    SEDX_CORE_WARN("Cannot get current video mode - no monitor available");
	    return nullptr;
	}
	
	/**
	 * @brief Gets the primary monitor width in pixels using SDL3
	 *
	 * @return Width in pixels or 0 if not available
	 */
	uint32_t MonitorData::GetWidth()
	{
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
	            SEDX_CORE_WARN("GetWidth: SDL video initialization failed");
	            return 0;
	        }
	    }
	
	    SDL_DisplayID display = SDL_GetPrimaryDisplay();
	    if (display == 0)
	    {
            SEDX_CORE_WARN("GetWidth: Primary display not available");
	        return 0;
	    }
	
	    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
	    if (!mode)
	    {
            SEDX_CORE_WARN("GetWidth: Failed to get primary display video mode");
	        return 0;
	    }
	
	    return static_cast<uint32_t>(mode->w);
	}
	
	/**
	 * @brief Gets the primary monitor height in pixels using SDL3
	 *
	 * @return Height in pixels or 0 if not available
	 */
	uint32_t MonitorData::GetHeight()
	{
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
                SEDX_CORE_WARN("GetHeight: SDL video initialization failed");
	            return 0;
	        }
	    }
	
	    SDL_DisplayID display = SDL_GetPrimaryDisplay();
	    if (display == 0)
	    {
            SEDX_CORE_WARN("GetHeight: Primary display not available");
	        return 0;
	    }
	
	    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
	    if (!mode)
	    {
            SEDX_CORE_WARN("GetHeight: Failed to get primary display video mode");
	        return 0;
	    }
	
	    return static_cast<uint32_t>(mode->h);
	}
	
	
	/**
	 * @brief Gets the primary monitor refresh rate in Hz using SDL3
	 *
	 * @return Refresh rate in Hz or 0.0f if not available
	 */
	float MonitorData::GetRefreshRate()
	{
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
                SEDX_CORE_WARN("GetRefreshRate: SDL video initialization failed");
	            return 0.0f;
	        }
	    }
	
	    SDL_DisplayID display = SDL_GetPrimaryDisplay();
	    if (display == 0)
	    {
            SEDX_CORE_WARN("GetRefreshRate: Primary display not available");
	        return 0.0f;
	    }
	
	    const SDL_DisplayMode *mode = SDL_GetCurrentDisplayMode(display);
	    if (!mode)
	    {
            SEDX_CORE_WARN("GetRefreshRate: Failed to get primary display video mode");
	        return 0.0f;
	    }
	
	    return mode->refresh_rate;
	}
	
	/**
	 * @brief Gets the primary monitor index using SDL3
	 *
	 * @return Monitor index within SDL_GetDisplays() list, 0 as safe fallback
	 */
	uint32_t MonitorData::GetId()
	{
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
                SEDX_CORE_WARN("GetId: SDL video initialization failed");
	            return 0;
	        }
	    }
	
	    int count = 0;
	    SDL_DisplayID *displays = SDL_GetDisplays(&count);
	    if (!displays || count <= 0)
	    {
            SEDX_CORE_WARN("GetId: No displays enumerated");
	        if (displays)
	        {
	            SDL_free(displays);
	        }

	        return 0;
	    }
	
	    SDL_DisplayID primary = SDL_GetPrimaryDisplay();
	    if (primary == 0)
	    {
            SEDX_CORE_WARN("GetId: Primary display not available");
	        SDL_free(displays);
	        return 0;
	    }
	
	    uint32_t result = 0;
	    for (int i = 0; i < count; ++i)
	    {
	        if (displays[i] == primary)
	        {
	            result = static_cast<uint32_t>(i);
	            break;
	        }
	    }
	
	    SDL_free(displays);
	    return result;
	}
	
	/** @brief Indicates HDR support using SDL3 provides HDR capability queries through display properties. */
	bool MonitorData::GetHdr()
	{
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
                SEDX_CORE_WARN("GetHdr: SDL video initialization failed");
	            return false;
	        }
	    }
	
	    SDL_DisplayID display = SDL_GetPrimaryDisplay();
	    if (display == 0)
	    {
            SEDX_CORE_WARN("GetHdr: Primary display not available");
	        return false;
	    }
	
	    SDL_PropertiesID props = SDL_GetDisplayProperties(display);
	    if (props == 0)
	    {
	        return false;
	    }
	
	    // Check if the display supports HDR
	    return SDL_GetBooleanProperty(props, SDL_PROP_DISPLAY_HDR_ENABLED_BOOLEAN, false);
	}
	
	/** @brief Gets the display's max luminance in nits using SDL3 provides HDR luminance information through display properties. */
	float MonitorData::GetLuminanceMax()
	{
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
                SEDX_CORE_WARN("GetLuminanceMax: SDL video initialization failed");
	            return 350.0f;
	        }
	    }
	
	    SDL_DisplayID display = SDL_GetPrimaryDisplay();
	    if (display == 0)
	    {
            SEDX_CORE_WARN("GetLuminanceMax: Primary display not available");
	        return 350.0f;
	    }
	
	    SDL_PropertiesID props = SDL_GetDisplayProperties(display);
	    if (props == 0)
	    {
	        return 350.0f;
	    }
	
	// Query SDR white level or max luminance
	// SDL_PROP_DISPLAY_SDR_WHITE_LEVEL_FLOAT may not be available in all SDL3 versions
	#ifdef SDL_PROP_DISPLAY_SDR_WHITE_LEVEL_FLOAT
	    float luminance = SDL_GetFloatProperty(props, SDL_PROP_DISPLAY_SDR_WHITE_LEVEL_FLOAT, 350.0f);
	#else
	    float luminance = 350.0f; // Default fallback
	#endif
	    return luminance;
	}
	
	/**
	 * @brief Gets the display's gamma
	 *
	 * SDL3 does not provide a direct gamma query. Return calibrated default.
	 */
	float MonitorData::GetGamma()
	{
	    // SDL3 doesn't provide gamma query, return default value
	    // This needs to be calibrated per display by the user
	    return 2.2f;
	}
	
	/**
	 * @brief Gets the primary monitor name using SDL3
	 *
	 * @return C-string of the monitor name or a fallback static string
	 */
	const char *MonitorData::GetName()
	{
	    if (!SDL_WasInit(SDL_INIT_VIDEO))
	    {
	        if (!SDL_Init(SDL_INIT_VIDEO))
	        {
                SEDX_CORE_WARN("GetName: SDL video initialization failed");
	            static const char *fallback = "Unknown Monitor";
	            return fallback;
	        }
	    }
	
	    SDL_DisplayID display = SDL_GetPrimaryDisplay();
	    if (display == 0)
	    {
            SEDX_CORE_WARN("GetName: Primary display not available");
	        static const char *fallback = "Unknown Monitor";
	        return fallback;
	    }
	
	    if (const char *name = SDL_GetDisplayName(display))
	        return name;
	
	    static const char *fallback = "Unknown Monitor";
	    return fallback;
	}
	
	void MonitorData::UpdateMonitorList()
	{
	    /**
		 * @brief UpdateMonitorList is reserved for future API expansion.
		 *
		 * Currently, all monitor enumeration and refresh logic is handled by RefreshMonitorList().
		 * This method exists in the public API for compatibility and future use cases where
		 * incremental or event-driven monitor updates may be required, distinct from a full refresh.
		 * For now, calling UpdateMonitorList() has no effect.
		 *
		 * See API documentation for details on when to use RefreshMonitorList() vs UpdateMonitorList().
		 */
	}
	
} // namespace SceneryEditorX

// --------------------------------------------

