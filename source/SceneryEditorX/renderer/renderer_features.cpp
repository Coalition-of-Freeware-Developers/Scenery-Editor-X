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
 * renderer_features.cpp
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#include "renderer_features.h"
#include <algorithm>
#include <array>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		using F = RendererFeatures;

		// A feature's bit is its index here: shaders test some of them (is_feature_enabled, FEATURE_* constants)
		constexpr std::array<RendererFeatureInfo, 21> FEATURES = { {
			{ "atmosphere", "Atmosphere", "Sky-view, transmittance and multi-scattering LUTs; the sky", 6, &F::atmosphere },
			{ "aerial-perspective", "Aerial perspective", "Haze and in-scattering between the camera and what it sees", 6, &F::aerialPerspective },
			{ "env-probes", "Environment probes", "Image-based lighting from GGX-filtered cube probes", 6, &F::envProbes },
			{ "cloud-shadows", "Cloud shadows", "World-space cloud shadow cascades", 7, &F::cloudShadows },
			{ "csm", "Sun shadows", "Cascaded shadow maps from the sun", 7, &F::csmShadows },
			{ "terrain", "Base terrain", "DSF terrain patches and .ter materials around the airport", 10, &F::terrain },
			{ "terrain-scenery", "Terrain scenery", "The base tile's own forests, roads, beaches and buildings within a mile of the airport", 10, &F::terrainScenery },
			{ "weather", "Weather surfaces", "Wetness, puddles, snow and ice applied to the g-buffer", 12, &F::weatherApply },
			{ "ao", "Ambient occlusion", "FidelityFX CACAO", 8, &F::ao },
			{ "tiled-lights", "Tiled lights", "Photometric night lighting: tiles of up to 512 lights, spill volumes", 14, &F::tiledLights },
			{ "ssr", "Screen-space reflections", "Hierarchical-Z reflections over the probes", 9, &F::ssr },
			{ "clouds", "Volumetric clouds", "Ray-marched cloud layers", 15, &F::clouds },
			{ "volumetric-fog", "Volumetric fog", "Froxel ground fog", 15, &F::volumetricFog },
			{ "water", "Water", "FFT ocean, tile colour, reflections", 11, &F::water },
			{ "auto-exposure", "Auto exposure", "Histogram metering of EV100", 5, &F::autoExposure },
			{ "exposure-fusion", "Exposure fusion", "Local tone mapping (dark interiors, bright skies)", 5, &F::exposureFusion },
			{ "bloom", "Bloom", "Energy-conserving glow around bright lights", 5, &F::bloom },
			{ "tonemap", "Tone mapping", "Exposure and the AgX output transform; off shows the pre-exposed frame as rendered", 3, &F::tonemap },
			{ "fxaa", "FXAA", "Post-process anti-aliasing", 5, &F::fxaa },
			{ "fsr1", "FSR 1", "Spatial upscaling (render below output resolution)", 5, &F::fsr1 },
			{ "sss", "Screen-space shadows", "Bend contact shadows (X-Plane 12.4 has none)", 7, &F::screenSpaceShadows },
		} };
	}

	uint32_t RendererFeatures::ToBits() const
	{
		uint32_t bits = 0;
		for (size_t i = 0; i < FEATURES.size(); ++i)
		{
			if (this->*FEATURES[i].member)
				bits |= 1u << i;
		}
		return bits;
	}

	RendererFeatures RendererFeatures::FromBits(const uint32_t bits)
	{
		RendererFeatures features;
		for (size_t i = 0; i < FEATURES.size(); ++i)
			features.*FEATURES[i].member = (bits & (1u << i)) != 0;
		return features;
	}

	RendererFeatures RendererFeatures::AllOff()
	{
		return FromBits(0);
	}

	std::span<const RendererFeatureInfo> GetRendererFeatureTable()
	{
		return FEATURES;
	}

	const RendererFeatureInfo *FindRendererFeature(const std::string_view name)
	{
		const auto it = std::ranges::find_if(FEATURES, [name](const RendererFeatureInfo &info) { return name == info.name; });
		return it == FEATURES.end() ? nullptr : &*it;
	}

	const char *GetDebugViewName(const Renderer_DebugView view)
	{
		static constexpr const char *names[static_cast<size_t>(Renderer_DebugView::Count)] = {
			"Off", "Albedo", "Roughness", "Metalness", "Normals", "Emissive (nits)", "Weather receptivity", "Ambient occlusion",
			"Shadow cascades", "Cloud shadow", "Reflections (SSR)", "Lights per tile", "Exposure (EV false colour)", "Luminance (nits)", "Depth"
		};
		const size_t index = static_cast<size_t>(view);
		return index < std::size(names) ? names[index] : "";
	}

	int GetDebugViewPhase(const Renderer_DebugView view)
	{
		switch (view)
		{
			case Renderer_DebugView::Emissive:
			case Renderer_DebugView::Weather:			return 4;
			case Renderer_DebugView::AmbientOcclusion:	return 8;
			case Renderer_DebugView::CsmCascades:
			case Renderer_DebugView::CloudShadow:		return 7;
			case Renderer_DebugView::Reflections:		return 9;
			case Renderer_DebugView::TileLightCount:	return 14;
			default:									return 0;
		}
	}
}

// -------------------------------------------------------
