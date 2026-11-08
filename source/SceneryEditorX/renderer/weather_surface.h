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
 * weather_surface.h
 * -------------------------------------------------------
 * Created: 25/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/environment/surface_weather.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	 * Weather on surfaces (plan Phase 12; XP12: weather_apply): wet ground, puddles, snow and ice, as weather.slang applies
	 * them to every surface it shades. A surface shows a kind where its receptivity passes the kind's threshold
	 * (Environment::WeatherThresholdCurve); X-Plane's art decides the receptivity (a WEATHER mask: R rain, G puddles,
	 * B snow; SNOW_ALBEDO_LUMA; WEATHER_TRANSPARENT), the environment's amounts the thresholds. Noise patches make the
	 * edges ragged; snow and ice take X-Plane's own textures (Resources/bitmaps/world/weather, loaded at runtime from the
	 * user's install), laid over the ground from above.
	 *
	 * The values are X-Plane 12.4.4's art controls under their names (the twxr and snow/luma controls, reference probe
	 * dumps 2026-10-25). Distances are read as meters per texture repeat (ASSUMPTION).
	 */
	struct WeatherSurfaceSettings
	{
		Environment::WeatherThresholdCurve rainThresholds = Environment::RAIN_THRESHOLDS;
		Environment::WeatherThresholdCurve puddleThresholds = Environment::PUDDLE_THRESHOLDS;
		Environment::WeatherThresholdCurve snowThresholds = Environment::SNOW_THRESHOLDS;
		Environment::WeatherThresholdCurve iceThresholds = Environment::ICE_THRESHOLDS;

		// Rain: where it wets (a noise over the ground), how much it darkens and smooths
		float rainAreaScaleM = 500.0f;				// twxr/rain_area_scale: the noise's repeat
		float rainAreaWidth = 0.15f;				// twxr/rain_area_width: soft edge of the threshold
		float rainNoiseModifier = 0.4f;				// twxr/rain_noise_modifier: added to the receptivity before the noise scales it
		float darkeningScale = 1.0f;				// twxr/rain_darkening_scale
		float darkeningStart = 0.0f;				// twxr/rain_darkening_start / _end: wetness over which darkening sets in
		float darkeningEnd = 0.35f;
		float darkeningStrength = 0.25f;			// twxr/rain_darkening_strength: bright surfaces darken less
		float darkeningRoughnessStart = 0.0f;		// twxr/rain_darkening_roughness_start / _end: gloss x multiplier ramp
		float darkeningRoughnessEnd = 0.7f;
		float roughnessMultiplierDark = 2.35f;		// twxr/rain_darkening_roughness_multiplier_dark / _light: by albedo luma
		float roughnessMultiplierLight = 1.35f;
		float darkenAlbedoRatio = 1.0f;				// twxr/rain_darken_albedo_rat
		float darkenGlossRatio = 0.75f;				// twxr/rain_darken_gloss_rat: share of the water film's gloss a wet surface takes
		float waterGloss = 0.95f;					// sedx/weather_water_gloss: the water film's gloss (X-Plane reads it from its animated rain droplets; ASSUMPTION without rain)

		// Snow
		float snowAreaScaleM = 500.0f;				// twxr/snow_area_scale
		float snowAreaWidth = 0.25f;				// twxr/snow_area_width
		float snowNoiseDepth = 0.04f;				// twxr/snow/noise_depth: how far the noise moves the receptivity
		float snowSlopeMin = 0.25f;					// twxr/snow/slope_min / slope_max: up . normal over which snow sets in
		float snowSlopeMax = 0.75f;
		float snowAlbedoM = 8.0f;					// twxr/snow/scale_albedo / scale_normal / scale_decal
		float snowNormalM = 0.25f;
		float snowDecalM = 42.0f;
		float snowDecal1K = 0.0f;					// twxr/snow/decal1_k: the decal's colour detail
		float snowDecal2K = 0.5f;					// twxr/snow/decal2_k: how much the decal breaks up the edge

		// Ice
		float iceAreaScaleM = 100.0f;				// twxr/ice_area_scale
		float iceAreaWidth = 0.75f;					// twxr/ice_area_width
		float iceAlbedoM = 40.0f;					// twxr/ice/scale_albedo / scale_normal / scale_decal
		float iceNormalM = 8.0f;
		float iceDecalM = 4.0f;
		float iceDecal1K = 1.0f;					// twxr/ice/decal1_k
		float iceDecal2K = 0.5f;					// twxr/ice/decal2_k

		// Snow receptivity of surfaces without a WEATHER mask: these weights dotted with their linear albedo (r, g, b, a)
		float snowLuma[4] = { 1.2f, 1.5f, 0.0f, 0.0f };	// snow/luma_r, luma_g, luma_b, luma_a
	};
}

// -------------------------------------------------------
