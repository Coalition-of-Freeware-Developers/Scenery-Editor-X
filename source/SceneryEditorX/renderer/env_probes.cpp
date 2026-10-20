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
 * env_probes.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "env_probes.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <numbers>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		double AngleDeg(const ProbeVec3 &a, const ProbeVec3 &b)
		{
			const double dot = a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
			return std::acos(std::clamp(dot, -1.0, 1.0)) * 180.0 / std::numbers::pi;
		}

		double Distance(const ProbeVec3 &a, const ProbeVec3 &b)
		{
			const double dx = a[0] - b[0], dy = a[1] - b[1], dz = a[2] - b[2];
			return std::sqrt(dx * dx + dy * dy + dz * dz);
		}
	}

	ProbeVec3 SnapProbePosition(const ProbeVec3 &camera, const double gridM)
	{
		if (gridM <= 0.0)
			return camera;
		return { std::round(camera[0] / gridM) * gridM, std::round(camera[1] / gridM) * gridM, std::round(camera[2] / gridM) * gridM };
	}

	bool EnvProbeNeedsCapture(const EnvProbeState &state, const EnvProbeInputs &inputs)
	{
		if (!state.valid || state.mediumHash != inputs.mediumHash)
			return true;
		if (Distance(state.position, inputs.camera) > ENV_PROBE_MOVE_M)
			return true;
		return AngleDeg(state.sun, inputs.sun) > ENV_PROBE_LIGHT_DEG || AngleDeg(state.moon, inputs.moon) > ENV_PROBE_LIGHT_DEG;
	}

	EnvProbeState CaptureEnvProbe(const EnvProbeInputs &inputs)
	{
		EnvProbeState state;
		state.valid = true;
		state.position = SnapProbePosition(inputs.camera);
		state.sun = inputs.sun;
		state.moon = inputs.moon;
		state.mediumHash = inputs.mediumHash;
		return state;
	}

	uint64_t HashCombine(uint64_t hash, const double value)
	{
		unsigned char bytes[sizeof(double)];
		std::memcpy(bytes, &value, sizeof(double));
		if (hash == 0)
			hash = 14695981039346656037ull;
		for (const unsigned char b : bytes)
		{
			hash ^= b;
			hash *= 1099511628211ull;
		}
		return hash;
	}
}

// -------------------------------------------------------
