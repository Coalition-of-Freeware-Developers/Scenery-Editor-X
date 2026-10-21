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
 * csm.cpp
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#include "csm.h"
#include <algorithm>
#include <cmath>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		double Dot(const CsmVec3 &a, const CsmVec3 &b)
		{
			return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
		}

		CsmVec3 Cross(const CsmVec3 &a, const CsmVec3 &b)
		{
			return { a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0] };
		}

		CsmVec3 Normalize(const CsmVec3 &v)
		{
			const double length = std::sqrt(Dot(v, v));
			return length > 0.0 ? CsmVec3{ v[0] / length, v[1] / length, v[2] / length } : CsmVec3{ 0.0, 0.0, 0.0 };
		}

		CsmVec3 Add(const CsmVec3 &a, const CsmVec3 &b, const double scale)
		{
			return { a[0] + b[0] * scale, a[1] + b[1] * scale, a[2] + b[2] * scale };
		}

		/* @brief Shadow-map texels a cascade's square keeps free around its sphere, for the PCF footprint (2 texels). */
		constexpr double MARGIN_TEXELS = 2.0;
	}

	std::array<double, CSM_MAX_CASCADES + 1> ComputeCsmSplits(const CsmSettings &settings, const double cameraNear)
	{
		std::array<double, CSM_MAX_CASCADES + 1> splits{};
		const uint32_t count = std::clamp(settings.cascadeCount, 1u, CSM_MAX_CASCADES);
		const double nearM = std::max(settings.nearLimitM, 1e-3);
		const double farM = std::max(settings.farLimitM, nearM * 1.001);
		const double lambda = std::clamp(settings.lambda, 0.0, 1.0);

		splits[0] = std::min(std::max(cameraNear, 0.0), farM);
		for (uint32_t i = 1; i < count; ++i)
		{
			const double t = static_cast<double>(i) / static_cast<double>(count);
			const double logarithmic = nearM * std::pow(farM / nearM, t);
			const double uniform = nearM + (farM - nearM) * t;
			splits[i] = std::max(lambda * logarithmic + (1.0 - lambda) * uniform, splits[i - 1]);
		}
		splits[count] = farM;
		return splits;
	}

	void CsmSliceSphere(const double depthNear, const double depthFar, const double tanHalfFovX, const double tanHalfFovY, double &centerDepth, double &radius)
	{
		// Corners of a slice at depth z lie on a ring of radius z x k around the view axis (k = the half-diagonal's tangent).
		// The sphere through both rings has its center at (n + f)(1 + k^2) / 2 on the axis; past f the far ring alone sets it.
		const double k2 = tanHalfFovX * tanHalfFovX + tanHalfFovY * tanHalfFovY;
		centerDepth = std::min(0.5 * (depthNear + depthFar) * (1.0 + k2), depthFar);
		const double toFar = depthFar - centerDepth;
		radius = std::sqrt(toFar * toFar + depthFar * depthFar * k2);
	}

	bool CsmCascade::CanCast(const CsmVec3 &boxMin, const CsmVec3 &boxMax) const
	{
		// The four sides and the far plane (index 5); the near plane (4) is left out: casters nearer the light still cast
		for (const size_t index : { 0u, 1u, 2u, 3u, 5u })
		{
			const std::array<double, 4> &plane = planes[index];
			// the box corner furthest along the plane's normal
			const CsmVec3 corner{ plane[0] >= 0.0 ? boxMax[0] : boxMin[0], plane[1] >= 0.0 ? boxMax[1] : boxMin[1], plane[2] >= 0.0 ? boxMax[2] : boxMin[2] };
			if (plane[0] * corner[0] + plane[1] * corner[1] + plane[2] * corner[2] + plane[3] < 0.0)
				return false;
		}
		return true;
	}

	CsmVec3 CsmCascade::Project(const CsmVec3 &world) const
	{
		const std::array<double, 16> &m = viewProjection;
		return { m[0] * world[0] + m[1] * world[1] + m[2] * world[2] + m[3],
				 m[4] * world[0] + m[5] * world[1] + m[6] * world[2] + m[7],
				 m[8] * world[0] + m[9] * world[1] + m[10] * world[2] + m[11] };
	}

	uint32_t CsmSetup::CascadeMask(const CsmVec3 &boxMin, const CsmVec3 &boxMax) const
	{
		uint32_t mask = 0;
		for (uint32_t i = 0; i < count; ++i)
		{
			if (cascades[i].CanCast(boxMin, boxMax))
				mask |= 1u << i;
		}
		return mask;
	}

	bool FitCsm(const CsmSettings &settings, const CsmCamera &camera, const CsmVec3 &towardLight, CsmSetup &out)
	{
		out = CsmSetup{};
		const CsmVec3 toLight = Normalize(towardLight);
		if (toLight[1] <= 1e-3 || settings.cascadeCount == 0 || settings.farLimitM <= 0.0)
			return false;

		const uint32_t count = std::clamp(settings.cascadeCount, 1u, CSM_MAX_CASCADES);
		const double mapSize = static_cast<double>(std::max(settings.mapSize, 16u));
		if (camera.orthographic)
		{
			// Screen size doesn't shrink with depth: even splits around the pivot (it can sit behind the eye's plane too)
			const double range = std::max(settings.farLimitM, 1.0);
			const double start = camera.focusDepth - 0.5 * range;
			for (uint32_t i = 0; i <= count; ++i)
				out.splits[i] = start + range * static_cast<double>(i) / static_cast<double>(count);
		}
		else
			out.splits = ComputeCsmSplits(settings, camera.nearPlane);
		if (out.splits[count] <= out.splits[0])
			return false;

		// The light's frame: it travels along forward; up stays as close to the world's up as the direction allows (the
		// world's south when the sun is within 2.6 degrees of the zenith)
		const CsmVec3 forward{ -toLight[0], -toLight[1], -toLight[2] };
		const CsmVec3 reference = std::abs(forward[1]) > 0.999 ? CsmVec3{ 0.0, 0.0, 1.0 } : CsmVec3{ 0.0, 1.0, 0.0 };
		const CsmVec3 right = Normalize(Cross(forward, reference));
		const CsmVec3 up = Cross(right, forward);
		out.lightRight = right;
		out.lightUp = up;
		out.lightForward = forward;

		const CsmVec3 viewAxis = Normalize(camera.forward);
		const double overlap = std::clamp(settings.overlapRatio, 0.0, 0.5);
		for (uint32_t i = 0; i < count; ++i)
		{
			CsmCascade &cascade = out.cascades[i];

			// This cascade's own range, plus the band where the previous one blends into it
			const double ownNear = out.splits[i];
			const double ownFar = out.splits[i + 1];
			out.blendBand[i] = (i + 1 < count) ? overlap * (ownFar - ownNear) : 0.0;
			cascade.depthNear = i > 0 ? std::max(ownNear - out.blendBand[i - 1], out.splits[0]) : ownNear;
			cascade.depthFar = ownFar;

			double centerDepth = 0.0, sphereRadius = 0.0;
			if (camera.orthographic)
			{
				// A box: half its depth and the view's half width and height
				const double halfDepth = 0.5 * (cascade.depthFar - cascade.depthNear);
				centerDepth = cascade.depthNear + halfDepth;
				sphereRadius = std::sqrt(halfDepth * halfDepth + camera.orthoHalfWidth * camera.orthoHalfWidth + camera.orthoHalfHeight * camera.orthoHalfHeight);
			}
			else
				CsmSliceSphere(cascade.depthNear, cascade.depthFar, camera.tanHalfFovX, camera.tanHalfFovY, centerDepth, sphereRadius);
			sphereRadius = std::max(sphereRadius, 1e-3);

			// The square around the sphere keeps a margin of texels for the filter; texel size follows from it
			const double radius = sphereRadius * mapSize / (mapSize - 2.0 * MARGIN_TEXELS);
			const double texel = 2.0 * radius / mapSize;

			// The center in the light's frame, snapped to whole texels across the light (along it, depth needs no snapping)
			const CsmVec3 sphereCenter = Add(camera.position, viewAxis, centerDepth);
			const double x = std::round(Dot(sphereCenter, right) / texel) * texel;
			const double y = std::round(Dot(sphereCenter, up) / texel) * texel;
			const double z = Dot(sphereCenter, forward);
			const double pad = std::max(settings.padNearConstM, 0.0) + std::max(settings.padNearRatio, 0.0) * radius;
			const double zNear = z - radius - pad;
			const double zFar = z + radius;

			cascade.center = Add(Add(Add(CsmVec3{ 0.0, 0.0, 0.0 }, right, x), up, y), forward, z);
			cascade.radius = radius;
			cascade.texelSize = texel;
			cascade.depthRange = zFar - zNear;

			// clip.x = (p.right - x) / r, clip.y = (p.up - y) / r, clip.z = (zFar - p.forward) / (zFar - zNear), w = 1
			const double inverseRange = 1.0 / cascade.depthRange;
			cascade.viewProjection = {
				right[0] / radius, right[1] / radius, right[2] / radius, -x / radius,
				up[0] / radius, up[1] / radius, up[2] / radius, -y / radius,
				-forward[0] * inverseRange, -forward[1] * inverseRange, -forward[2] * inverseRange, zFar * inverseRange,
				0.0, 0.0, 0.0, 1.0 };

			// The clip planes as combinations of the rows (the same as gpu_cull.slang derives on the GPU)
			const std::array<double, 16> &m = cascade.viewProjection;
			const auto row = [&m](const int r, const int c) { return m[static_cast<size_t>(r * 4 + c)]; };
			const auto plane = [&row](const int r, const double sign)
			{
				return std::array<double, 4>{ row(3, 0) + sign * row(r, 0), row(3, 1) + sign * row(r, 1), row(3, 2) + sign * row(r, 2), row(3, 3) + sign * row(r, 3) };
			};
			cascade.planes = { plane(0, 1.0), plane(0, -1.0), plane(1, 1.0), plane(1, -1.0), plane(2, -1.0),
							   std::array<double, 4>{ row(2, 0), row(2, 1), row(2, 2), row(2, 3) } };
		}

		const double farM = out.splits[count];
		out.fadeEnd = farM;
		out.fadeStart = camera.orthographic ? out.splits[0] + std::clamp(settings.fadeRatio, 0.0, 1.0) * (farM - out.splits[0])
											: std::clamp(settings.fadeRatio, 0.0, 1.0) * farM;
		out.count = count;
		return true;
	}

	CloudShadowCascade FitCloudShadowCascade(const uint32_t index, const uint32_t count, const double minExtentM, const double maxExtentM, const double cameraX,
		const double cameraZ, const uint32_t size)
	{
		const double smallest = std::max(minExtentM, 1.0);
		const double largest = std::max(maxExtentM, smallest);
		const double t = count > 1 ? static_cast<double>(std::min(index, count - 1)) / static_cast<double>(count - 1) : 0.0;
		const double extent = smallest * std::pow(largest / smallest, t);

		CloudShadowCascade cascade;
		cascade.halfExtentM = 0.5 * extent;
		cascade.texelM = extent / static_cast<double>(std::max(size, 1u));
		cascade.centerX = std::round(cameraX / cascade.texelM) * cascade.texelM;
		cascade.centerZ = std::round(cameraZ / cascade.texelM) * cascade.texelM;
		return cascade;
	}
}

// -------------------------------------------------------
