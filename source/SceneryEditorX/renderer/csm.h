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
 * csm.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <cstdint>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	 * Cascaded sun shadows (plan Phase 7, TASK-052; XP12: the CSM the shadow/csm art controls drive): where the
	 * cascades split the view, the bounding sphere of each slice of the view frustum, and the light's orthographic
	 * projection of it. Standard library only, unit tested; Renderer::Pass_ShadowCascades draws them and
	 * csm_sample.slang reads them.
	 *
	 * Each cascade's sphere depends only on its slice's depths and the field of view, so its size never changes while the
	 * camera turns or moves, and its center is snapped to whole shadow-map texels in the light's frame: shadow edges
	 * don't shimmer while the camera pans (plan CRIT-007). The world frame is the renderer's (+X east, +Y up, +Z south).
	 */
	inline constexpr uint32_t CSM_MAX_CASCADES = 4;

	using CsmVec3 = std::array<double, 3>;

	/* @brief X-Plane's shadow art controls; defaults are X-Plane 12.4.4's (reference probe dump, 2026-10-20). */
	struct CsmSettings
	{
		uint32_t cascadeCount = 3;		// shadow/csm_split_exterior (renopt_shadow_quality)
		double nearLimitM = 4.0;		// shadow/csm/near_limit_exterior: the split scheme's near distance
		double farLimitM = 250.0;		// shadow/csm/far_limit_exterior (renopt_shadow_quality): no sun shadows past it
		double lambda = 1.0;			// shadow/csm/lambda: 1 logarithmic .. 0 uniform splits
		double overlapRatio = 0.01;		// shadow/overlap_ratio: band each cascade blends into the next, share of its depth
		double fadeRatio = 0.7;			// shadow/total_fade_ratio: shadows fade out from this share of the far limit on
		double padNearConstM = 100.0;	// shadow/csm/pad_near_const: the near plane moved toward the light (m)
		double padNearRatio = 1.0;		// shadow/csm/pad_near_ratio: plus this share of the cascade's radius (ASSUMPTION)
		uint32_t mapSize = 2048;		// fbo/shadow_cam_size (renopt_shadow_quality)
	};

	/* @brief The view the cascades cover: a symmetric perspective camera, or an orthographic one. */
	struct CsmCamera
	{
		CsmVec3 position{};				// World (m)
		CsmVec3 forward{ 0.0, 0.0, -1.0 };	// Unit, world
		double tanHalfFovX = 0.57735;	// tan(horizontal field of view / 2)
		double tanHalfFovY = 0.32476;
		double nearPlane = 0.1;			// m

		// Orthographic: every slice is a box of the view's half width and height. What it shows sits around the orbit pivot,
		// whatever the eye's distance, so the cascades split depths evenly over the far limit centered on focusDepth.
		bool orthographic = false;
		double orthoHalfWidth = 50.0;	// m
		double orthoHalfHeight = 50.0;
		double focusDepth = 0.0;		// View depth of the orbit pivot (m)
	};

	/* @brief One cascade: the slice of the view it serves and the light's projection of it. */
	struct CsmCascade
	{
		double depthNear = 0.0;			// View depth (m, along the camera's forward) the projection covers, the previous
		double depthFar = 0.0;			// cascade's blend band included
		CsmVec3 center{};				// The slice's bounding sphere (world), center snapped to whole texels
		double radius = 0.0;			// Half the side of the projected square (m): the sphere's radius plus a texel margin
		double texelSize = 0.0;			// m per shadow-map texel
		double depthRange = 0.0;		// m from the near plane (toward the light, padded) to the far plane

		/* @brief The light's view-projection as the GPU takes it: the rows of the column-vector matrix (clip = M * p),
		 * orthographic, x / y in [-1, 1] (+Y up), reverse-Z: 1 at the near plane (toward the light), 0 at the far one. */
		std::array<double, 16> viewProjection{};

		/* @brief left, right, bottom, top, near, far: inside where dot(xyz, p) + w >= 0 (unnormalized). */
		std::array<std::array<double, 4>, 6> planes{};

		/**
		 * @brief Whether an axis-aligned world box can cast into this cascade: inside its four sides and in front of its
		 * far plane. Casters nearer the light than the near plane still cast (they are clamped onto it while drawn).
		 */
		[[nodiscard]] bool CanCast(const CsmVec3 &boxMin, const CsmVec3 &boxMax) const;

		/* @brief clip = M * (p, 1) for a world point (w is 1). */
		[[nodiscard]] CsmVec3 Project(const CsmVec3 &world) const;
	};

	/* @brief The cascades for one frame. */
	struct CsmSetup
	{
		uint32_t count = 0;				// 0: no sun shadows (no light, or the far limit is behind the near plane)
		std::array<CsmCascade, CSM_MAX_CASCADES> cascades{};
		std::array<double, CSM_MAX_CASCADES + 1> splits{};	// View depths: cascade i serves splits[i] .. splits[i + 1]
		double fadeStart = 0.0;			// View depth where shadows start to fade out
		double fadeEnd = 0.0;			// And are gone (the far limit)
		double blendBand[CSM_MAX_CASCADES] = {};	// Depth over which cascade i blends into i + 1 before splits[i + 1]
		CsmVec3 lightRight{ 1.0, 0.0, 0.0 };	// The light's frame: right, up, and the direction light travels
		CsmVec3 lightUp{ 0.0, 1.0, 0.0 };
		CsmVec3 lightForward{ 0.0, -1.0, 0.0 };

		/* @brief Bit i set when the box can cast into cascade i. */
		[[nodiscard]] uint32_t CascadeMask(const CsmVec3 &boxMin, const CsmVec3 &boxMax) const;
	};

	/**
	 * @brief Practical split scheme (Zhang et al. 2006): split i = lambda x the logarithmic split + (1 - lambda) x the
	 * uniform one, between nearLimitM and farLimitM. splits[0] is the camera's near plane (the first cascade starts at
	 * the camera), splits[count] the far limit; entries past count are 0.
	 */
	std::array<double, CSM_MAX_CASCADES + 1> ComputeCsmSplits(const CsmSettings &settings, double cameraNear);

	/**
	 * @brief Bounding sphere of the view-frustum slice between two view depths: its distance along the view axis and
	 * radius (the smallest sphere through both rings of corners, or around the far ring when that one is larger).
	 */
	void CsmSliceSphere(double depthNear, double depthFar, double tanHalfFovX, double tanHalfFovY, double &centerDepth, double &radius);

	/**
	 * @brief Fits the cascades for a camera and a light.
	 * @param towardLight Unit vector toward the sun (or moon); below the horizon there are no shadows (count 0).
	 * @return False (count 0) when nothing casts: no light above the horizon or no cascades.
	 */
	bool FitCsm(const CsmSettings &settings, const CsmCamera &camera, const CsmVec3 &towardLight, CsmSetup &out);

	/*
	 * Cloud shadows (plan TASK-055; XP12: clouds/shadow_size 1024, cloud/shadow_cascades 2, clouds/shadow_extent_min_km
	 * 64, shadow_extent_km 256): square world-space maps around the camera of the light that comes through the clouds,
	 * one per cascade, the widths spread geometrically from the smallest to the largest (cloud_shadows.slang).
	 */
	inline constexpr uint32_t CLOUD_SHADOW_SIZE = 1024;
	inline constexpr uint32_t CLOUD_SHADOW_CASCADES = 2;

	struct CloudShadowCascade
	{
		double centerX = 0.0;			// World (m), snapped to whole texels
		double centerZ = 0.0;
		double halfExtentM = 0.0;
		double texelM = 0.0;
	};

	/* @brief Cloud-shadow cascade index of count around a camera's xz. */
	CloudShadowCascade FitCloudShadowCascade(uint32_t index, uint32_t count, double minExtentM, double maxExtentM, double cameraX, double cameraZ,
		uint32_t size = CLOUD_SHADOW_SIZE);
}

// -------------------------------------------------------
