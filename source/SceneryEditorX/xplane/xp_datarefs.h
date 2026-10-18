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
 * xp_datarefs.h
 * -------------------------------------------------------
 * Created: 17/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_obj8.h"
#include <map>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Emulated X-Plane datarefs for previewing object animation in the editor.
	 *
	 * X-Plane drives OBJ animation from datarefs (simulator variables). The editor has no simulator, so each
	 * dataref an object reads gets a control: a fixed value, a sweep across the object's keyframe range, or the
	 * sim clock. A few common scenery datarefs (time, wind, lighting) are derived from the environment settings.
	 */

	/* @brief How a dataref's value is produced. */
	enum class DatarefDriver : uint8_t
	{
		Value,		// Fixed value set by the user
		Sweep,		// Ping-pongs between min and max over `period` seconds of sim time
		Clock,		// Sim time in seconds multiplied by `rate` (ever increasing, for looping animations)
		Builtin		// "Auto": derived from the environment (sim clock, time of day, wind, lighting)
	};

	const char *GetDatarefDriverName(DatarefDriver driver);

	/* @brief The editor-side control for one dataref (key: "name" or "name[index]"). */
	struct DatarefControl
	{
		std::string key;
		DatarefDriver driver = DatarefDriver::Value;
		float value = 0.0f;			// Value driver; also the last evaluated value for display
		float minValue = 0.0f;		// Range from the objects that read the dataref (sliders, sweeps)
		float maxValue = 1.0f;
		float period = 4.0f;		// Sweep period in seconds
		float rate = 1.0f;			// Clock multiplier
		uint32_t users = 0;			// Objects that registered the dataref
	};

	/* @brief Environment the built-in datarefs derive from. */
	struct DatarefEnvironment
	{
		float localTimeHours = 12.0f;	// 0..24
		float windDirectionDeg = 270.0f;	// Direction the wind blows FROM, true degrees
		float windSpeedKnots = 8.0f;
		bool forceLightsOn = false;		// Show LIT textures and lights in daylight
	};

	/**
	 * @class Datarefs
	 * @brief Global dataref table (X-Plane datarefs are global too). Main thread only.
	 */
	class Datarefs
	{
	public:
		/* @brief Advances the sim clock (when playing) and refreshes driven values. */
		static void Tick(double deltaSeconds);

		/* @brief Current value of a dataref; unknown datarefs read 0 like an unset dataref in X-Plane. */
		static float Get(const DatarefRef &ref);

		/* @brief Reader for animation evaluation. */
		static DatarefReader GetReader();

		/**
		 * @brief Makes sure a dataref an object reads has a control, widening its range to the object's keyframes.
		 * New controls pick a sensible default driver (built-in, clock for time datarefs, sweep for sim/graphics/animation).
		 */
		static DatarefControl &Register(const Obj8DatarefUsage &usage);

		/* @brief The control for a key, or nullptr. */
		static DatarefControl *Find(const std::string &key);

		/* @brief Every registered control, ordered by key. */
		static std::map<std::string, DatarefControl> &GetControls();

		/* @brief Sets a control to a fixed value (switches it to the Value driver). */
		static void SetValue(const std::string &key, float value);

		static bool IsPlaying();
		static void SetPlaying(bool playing);
		static float GetTimeScale();
		static void SetTimeScale(float scale);
		static double GetSimTime();
		static void ResetClock();

		static DatarefEnvironment &GetEnvironment();

		/* @brief 0 in daylight, 1 at night (from the local time); 1 when lights are forced on. */
		static float GetNightFactor();

		/* @brief How much sunlight there is (1 by day, 0 at night) from the local time, whatever forceLightsOn says. */
		static float GetDaylight();

		/* @brief True when the given dataref is computed from the environment rather than a control. */
		static bool IsBuiltin(std::string_view name);
	};

}

// -------------------------------------------------------
