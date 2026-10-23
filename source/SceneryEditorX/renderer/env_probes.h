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
 * env_probes.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	 * Environment probes (plan TASK-049, REQ-005): a cube array of up to 8 probes, RGBA16F, 7 mips (cube_filter.slang).
	 * Probe 0 sits at the camera snapped to a grid and is captured again only when what it sees changes. Standard library
	 * only: the rules here are unit tested; Renderer::Pass_EnvProbes records the captures.
	 */
	inline constexpr uint32_t ENV_PROBE_SIZE = 128;		// lighting/probe_resolution (ASSUMPTION until the probe dumps it)
	inline constexpr uint32_t ENV_PROBE_COUNT = 8;
	inline constexpr uint32_t ENV_PROBE_MIPS = 7;		// 0 radiance, 1-5 GGX roughness mip / 5, 6 cosine (irradiance / pi)
	inline constexpr double ENV_PROBE_GRID_M = 50.0;
	inline constexpr double ENV_PROBE_MOVE_M = 200.0;	// A probe further than this from the camera is captured again
	inline constexpr double ENV_PROBE_LIGHT_DEG = 0.5;	// So is one whose sun or moon moved more than this

	using ProbeVec3 = std::array<double, 3>;

	/* @brief What a capture depends on. */
	struct EnvProbeInputs
	{
		ProbeVec3 camera{};			// World (meters)
		ProbeVec3 sun{ 0.0, 1.0, 0.0 };	// Unit, toward the sun
		ProbeVec3 moon{ 0.0, 1.0, 0.0 };
		uint64_t mediumHash = 0;	// The atmosphere's medium and light outside it (visibility, sky condition, illuminance)
	};

	/* @brief Probe 0 as last captured. */
	struct EnvProbeState
	{
		bool valid = false;
		ProbeVec3 position{};		// Snapped
		ProbeVec3 sun{ 0.0, 1.0, 0.0 };
		ProbeVec3 moon{ 0.0, 1.0, 0.0 };
		uint64_t mediumHash = 0;
	};

	/* @brief The camera snapped to the probe grid (each axis to the nearest multiple of grid meters). */
	ProbeVec3 SnapProbePosition(const ProbeVec3 &camera, double gridM = ENV_PROBE_GRID_M);

	/* @brief Whether probe 0 must be captured again: never captured, the medium changed, the camera moved more than
	 * ENV_PROBE_MOVE_M from it, or the sun or moon turned more than ENV_PROBE_LIGHT_DEG. */
	bool EnvProbeNeedsCapture(const EnvProbeState &state, const EnvProbeInputs &inputs);

	/* @brief The state after a capture with these inputs. */
	EnvProbeState CaptureEnvProbe(const EnvProbeInputs &inputs);

	/* @brief A 64-bit FNV-1a step over a value's bytes, for EnvProbeInputs::mediumHash. */
	uint64_t HashCombine(uint64_t hash, double value);
}

// -------------------------------------------------------
