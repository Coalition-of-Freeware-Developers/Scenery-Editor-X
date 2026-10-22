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
 * renderer_features.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <span>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @struct RendererFeatures
	 * @brief The passes of the XP12 emulation frame graph that can be switched off (plan TASK-021): the RENDERER panel's
	 * "XP12 EMULATION" section and --no-<name> on the command line, for A/B comparisons in RenderDoc and the parity
	 * harness. A feature whose pass hasn't been written yet (GetFeatureTable: implementedInPhase in the future) has no
	 * effect either way.
	 */
	struct RendererFeatures
	{
		bool atmosphere = true;			// Sky LUTs and the sky (Phase 6)
		bool aerialPerspective = true;	// Haze between the camera and surfaces (Phase 6)
		bool envProbes = true;			// Image-based lighting from cube probes (Phase 6)
		bool csmShadows = true;			// Cascaded sun shadows (Phase 7)
		bool cloudShadows = true;		// (Phase 7)
		bool ao = true;					// Ambient occlusion, CACAO (Phase 8)
		bool ssr = true;				// Screen-space reflections (Phase 9)
		bool terrain = true;			// DSF base terrain (Phase 10)
		bool terrainScenery = true;		// The base tile's forests, roads, beaches and buildings on the terrain (Phase 10)
		bool autogen = true;			// The base tile's autogen with that scenery (Phase 10)
		bool water = true;				// (Phase 11)
		bool weatherApply = true;		// Wet, puddles, snow, ice on the g-buffer (Phase 12)
		bool tiledLights = true;		// Photometric night lighting (Phase 14)
		bool clouds = true;				// Volumetric clouds (Phase 15)
		bool volumetricFog = true;		// (Phase 15)
		bool autoExposure = true;		// Histogram metering (Phase 5)
		bool exposureFusion = true;		// Local tone mapping (Phase 5)
		bool bloom = true;				// (Phase 5)
		bool tonemap = true;			// AgX output transform; off: the frame is shown as rendered (pre-exposed)
		bool fxaa = true;				// (Phase 5)
		bool fsr1 = false;				// FSR 1 upscaling (Phase 5; off in X-Plane's default preset)
		bool screenSpaceShadows = false;	// Bend screen-space shadows: X-Plane 12.4 has none (Phase 7)

		[[nodiscard]] uint32_t ToBits() const;
		static RendererFeatures FromBits(uint32_t bits);
		static RendererFeatures AllOff();

		bool operator==(const RendererFeatures &) const = default;
	};

	/* @brief A feature's names and where the plan implements it. */
	struct RendererFeatureInfo
	{
		const char *name;				// Command line: --no-<name>
		const char *label;				// UI
		const char *description;
		int implementedInPhase;			// Plan phase whose pass makes the feature do something
		bool RendererFeatures::*member;
	};

	/* @brief Every feature, in frame-graph order. */
	std::span<const RendererFeatureInfo> GetRendererFeatureTable();

	/* @brief The feature named name (the --no-<name> spelling), or nullptr. */
	const RendererFeatureInfo *FindRendererFeature(std::string_view name);

	/* @brief The latest plan phase whose passes are in this build: features of later phases have no effect yet. */
	inline constexpr int RENDERER_IMPLEMENTED_PHASE = 10;

	/**
	 * @brief What the RENDERER panel's DEBUG VIEW shows instead of the frame (plan TASK-026); debug_view.slang.
	 * Views whose data arrives in a later phase show black until then.
	 */
	enum class Renderer_DebugView : uint8_t
	{
		None,
		Albedo,
		Roughness,
		Metalness,
		Normals,
		Emissive,			// Phase 4: emissive nits
		Weather,			// Phase 4 / 12: weather receptivity
		AmbientOcclusion,	// Phase 8
		CsmCascades,		// Phase 7
		CloudShadow,		// Phase 7
		Reflections,		// Phase 9: SSR
		TileLightCount,		// Phase 14
		ExposureFalseColor,	// EV of every pixel as colour bands
		LuminanceNits,		// Luminance of the frame (divided by the pre-exposure), log scaled
		Depth,
		Count
	};

	/* @brief "Albedo", "Roughness" ... */
	const char *GetDebugViewName(Renderer_DebugView view);

	/* @brief The plan phase that makes a view show something (0: available now). */
	int GetDebugViewPhase(Renderer_DebugView view);
}

// -------------------------------------------------------
