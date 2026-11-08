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
 * ocean.h
 * -------------------------------------------------------
 * Created: 22/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <complex>
#include <cstdint>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/*
	 * Water (plan Phase 11, TASK-071 to 073; XP12: ocean_spectrum, compute_fft, ocean_bake_textures, ocean_variance,
	 * ocean_shading): the CPU half, standard library only and unit tested. X-Plane's water/* art controls and SEDX's own,
	 * the sea state the environment's wave height turns into, the wave spectrum the GPU fills its cascades from
	 * (ocean_common.slang keeps the same functions; keep the two alike), a CPU twin of the GPU's inverse FFT, and the
	 * water's optics and tessellation rules.
	 *
	 * The spectrum follows the published formulas of Horvath, "Empirical directional wave spectra for computer graphics"
	 * (DigiPro 2015), which X-Plane's ocean_spectrum parameters (u_peak_omega, u_gamma, u_swell, u_spread_blend,
	 * u_short_waves_fade, u_depth) match: JONSWAP (Hasselmann et al. 1973) with the TMA shallow-water factor (Bouws et al.
	 * 1985), Hasselmann's directional spreading with Horvath's swell term, blended toward uniform by the spread blend. The
	 * surface is Tessendorf's ("Simulating Ocean Water", 2001): h(k, t) = h0(k) e^(-i w t) + conj(h0(-k)) e^(i w t), choppy
	 * horizontal displacement -i k / |k| h, summed over four cascades of 256 x 256 waves each covering its own band of
	 * wave numbers. Distant water is filtered by slope variance kept in the mips (LEAN mapping: Olano and Baker 2010, the
	 * same idea as Bruneton et al. 2010's ocean lighting).
	 */

	inline constexpr uint32_t OCEAN_FFT_SIZE = 256;		// Waves per cascade side (X-Plane: 256-point FFTs)
	inline constexpr uint32_t OCEAN_CASCADES = 4;		// water/cascade_0..3_scale
	inline constexpr float OCEAN_GRAVITY = 9.81f;
	inline constexpr float OCEAN_PI = 3.14159265358979f;

	/**
	 * @brief X-Plane's water controls (sim/private/controls/water/*, X-Plane 12.4.4's values from the probe dump) and SEDX's
	 * (sedx/water_*). ASSUMPTION marks the ones no reference has checked yet (plan Phase 16).
	 */
	struct OceanSettings
	{
		// Waves
		std::array<float, OCEAN_CASCADES> cascadeScale = { 1.0f, 1.0f, 1.0f, 1.0f };	// water/cascade_0..3_scale
		std::array<float, OCEAN_CASCADES> cascadeSizeM = { 500.0f, 120.0f, 30.0f, 7.0f };	// sedx/water_cascade_size_0..3: tile of each cascade (ASSUMPTION; X-Plane's aren't exposed)
		float cascadeMinCycles = 4.0f;		// sedx/water_cascade_min_cycles: a cascade starts at waves this many times across its tile (the larger one holds longer waves)
		float fetchLoM = 1000.0f;			// water/spectrum/fetch_lo_m: fetch at height_lo_m
		float fetchHiM = 5000.0f;			// water/spectrum/fetch_hi_m: fetch at height_hi_m
		float fetchMultiplier = 10.0f;		// water/spectrum/fetch_multiplier
		float heightLoM = 0.3048f;			// water/spectrum/height_lo_m (1 ft)
		float heightHiM = 1.8288f;			// water/spectrum/height_hi_m (6 ft)
		float heightNoSwellM = 0.1524f;		// water/spectrum/height_no_swell_m: no swell at or below this height
		float heightSwellM = 1.8288f;		// water/spectrum/height_swell_m: max_swell at or above this height
		float maxSwell = 0.8f;				// water/spectrum/max_swell
		float spreadBlend = 1.0f;			// water/spectrum/spread_blend: 0 waves in every direction, 1 the directional model
		float shortWavesFade = 0.01f;		// water/spectrum/short_waves_fade: waves shorter than about this (m) fade out
		float displaceLambda = 1.0f;		// water/h_displace_lambda: choppiness (horizontal displacement)
		float gamma = 3.3f;					// JONSWAP peak enhancement (X-Plane's u_gamma; 3.3 is the JONSWAP mean)
		float spectrumDepthM = 100.0f;		// sedx/water_spectrum_depth_m: depth the dispersion and the TMA factor assume (ASSUMPTION)
		float heightFromAmplitude = 2.0f;	// sedx/water_height_per_amplitude: significant wave height per meter of wave_amplitude (ASSUMPTION)

		// Surface and volume
		float f0 = 0.02f;					// water/F0
		float gloss = 0.78f;				// water/gloss: the surface's own gloss, before the waves' slope variance
		float turbidity = 2.0f;				// water/turbidity
		float turbidityCutoff = -4.605170f;	// water/turbidity/cutoff: ln of the transmittance at the clarity distance (1%)
		bool enableTurbidity = true;		// water/enable_turbidity
		float foamBias = 0.8f;				// water/foam_bias: the waves' Jacobian below which they break
		float foamRatio = 2.0f;				// water/foam_ratio: foam per unit of Jacobian below the bias (ASSUMPTION: meaning)
		float foamScale = 2.4f;				// water/foam_scale: foam texture gain (ASSUMPTION: meaning)
		float foamDecay = 0.6f;				// sedx/water_foam_decay: share of the foam that fades per second (ASSUMPTION)
		float refractiveIndex = 1.333f;		// sedx/water_ior
		float seaFloorCoef = 0.0f;			// water/sea_floor_coef: share of the sea floor seen through the water (0 in 12.4.4, whose lakes show none: ASSUMPTION about its meaning)
		std::array<float, 3> floorAlbedo = { 0.30f, 0.27f, 0.20f };	// sedx/water_floor_albedo: the sea floor under the floor decal (ASSUMPTION)
		float floorDecalMeters = 25.0f;		// sedx/water_floor_decal_m: meters one repeat of the floor decal covers (ASSUMPTION)
		float clarityMinM = 0.5f;			// sedx/water_clarity_min_m: clearest-water floor for a tile alpha of 0

		// Geometry
		float shoreDepthM = 0.5f;			// sedx/water_shore_depth_m: waves rise to full height over this floor depth (keeps shores in place; lakes' bathymetry is often 1-2 m)
		/*
		 * Whether the DSF's per-vertex fetch ratio (1 open ocean .. 0 ponds) scales the waves. X-Plane 12.4.4's dsf/kill_fetch
		 * is 1, but its references show calm lakes mirroring their shores under the same sea that fills the sound, so SEDX
		 * keeps the DSF spec's meaning (ASSUMPTION; sedx/water_use_fetch).
		 */
		bool useFetch = true;
		float pondWaveScale = 0.08f;		// sedx/water_pond_wave_scale: wave height where the fetch ratio is 0 (a few hundred meters of fetch: ~0.09 m under a 1 m sea), 1 at fetch 1
		float gridCellM = 64.0f;			// sedx/water_grid_m: water meshes are cut into cells this size before tessellation
		float tessellationPixels = 6.0f;	// sedx/water_tessellation_px: target length of a tessellated edge on screen
		float maxTessellation = 64.0f;		// hardware limit
	};

	/** @brief The sea state for the spectrum: what X-Plane's u_ocean_spectrum_data carries. */
	struct OceanSpectrum
	{
		float significantHeightM = 0.0f;	// Hs the cascades are scaled to (4 x the surface's RMS height)
		float windSpeed = 0.0f;				// m/s at 10 m
		float fetchM = 0.0f;
		float swell = 0.0f;					// Horvath's xi: 0 wind sea .. 1 long-crested swell
		float spreadBlend = 1.0f;
		float shortWavesFade = 0.0f;		// m
		float depthM = 100.0f;
		float alpha = 0.0f;					// JONSWAP energy scale
		float peakOmega = 0.0f;				// rad/s
		float gamma = 3.3f;
		float directionRad = 0.0f;			// Direction the waves travel, in the ground plane: atan2(z, x) (x east, z south)
		float heightScale = 0.0f;			// Multiplies every amplitude so the cascades' Hs is significantHeightM

		bool operator==(const OceanSpectrum &) const = default;
	};

	/** @brief One cascade: its tile and the band of wave numbers it holds (rad/m). */
	struct OceanCascade
	{
		float sizeM = 0.0f;
		float kMin = 0.0f;					// Inclusive
		float kMax = 0.0f;					// Exclusive (the last cascade: its Nyquist limit)
		float scale = 1.0f;					// water/cascade_N_scale
	};

	/** @brief The four cascades' tiles and wave-number bands: contiguous, none shorter than its own grid can hold. */
	std::array<OceanCascade, OCEAN_CASCADES> ComputeOceanCascades(const OceanSettings &settings);

	/**
	 * @brief The sea state for a wave amplitude (sim/weather/region/wave_amplitude, m) from a direction (wave_dir, deg true,
	 * where the waves come from), the way X-Plane says it works: the height sets the wave length and speed too. Height
	 * picks the fetch (fetch_lo_m .. fetch_hi_m x fetch_multiplier) and the swell (0 .. max_swell); the wind speed is
	 * the one whose fetch-limited JONSWAP sea has that height (Hs = 0.0016 U sqrt(F / g)), at least the fully developed
	 * sea's (Hs = 0.21 U^2 / g). heightScale is left at 0: NormalizeOceanSpectrum sets it.
	 */
	OceanSpectrum ComputeOceanSpectrum(float waveAmplitudeM, float waveDirectionDegT, const OceanSettings &settings);

	/** @brief Angular frequency (rad/s) of waves of wave number k (rad/m) in water depthM deep: w^2 = g k tanh(k d). */
	float OceanDispersion(float k, float depthM);

	/** @brief dw / dk, the group speed (m/s). */
	float OceanDispersionDerivative(float k, float depthM);

	/** @brief JONSWAP frequency spectrum S(w) (m^2 s). */
	float JonswapSpectrum(float omega, const OceanSpectrum &spectrum);

	/** @brief TMA shallow-water factor (Kitaigorodskii's, as Horvath approximates it): 1 in deep water. */
	float TmaAttenuation(float omega, float depthM);

	/**
	 * @brief Directional spreading D(w, theta) (per radian, integrates to 1 over -pi..pi): Hasselmann's cos-2s spreading
	 * with Horvath's swell term (s + 16 tanh(wp / w) xi^2), blended toward uniform by the spread blend. theta is the angle
	 * between a wave's direction and the sea's.
	 */
	float OceanDirectionalSpread(float omega, float theta, const OceanSpectrum &spectrum);

	/**
	 * @brief The wave-number spectrum E(kx, kz) (m^4: height variance per unit of k-space area) the cascades sample:
	 * S(w) TMA(w) D(w, theta) (dw / dk) / k, short waves faded by exp(-(k l)^2). Without heightScale.
	 */
	float OceanSpectrumEnergy(float kx, float kz, const OceanSpectrum &spectrum);

	/** @brief The wave number (rad/m) of grid cell n (0..size-1) of a cascade, in the FFT's order (n >= size / 2: negative). */
	float OceanGridWaveNumber(uint32_t n, float cascadeSizeM, uint32_t size = OCEAN_FFT_SIZE);

	/**
	 * @brief Significant wave height (m) of the surface the cascades make, before heightScale: 4 x the square root of the
	 * variance summed over every cell each cascade keeps (E(k) dk^2), exactly what the GPU's grids hold.
	 */
	float OceanCascadesSignificantHeight(const OceanSpectrum &spectrum, const std::array<OceanCascade, OCEAN_CASCADES> &cascades, uint32_t size = OCEAN_FFT_SIZE);

	/** @brief Sets heightScale so the cascades' significant height is spectrum.significantHeightM (0 when there are no waves). */
	void NormalizeOceanSpectrum(OceanSpectrum &spectrum, const std::array<OceanCascade, OCEAN_CASCADES> &cascades, uint32_t size = OCEAN_FFT_SIZE);

	/**
	 * @brief CPU twin of ocean.slang's inverse FFT, in place: the input in bit-reversed order, then log2(n) radix-2
	 * butterflies with twiddles e^(+2 pi i j / m), no 1 / n (the spectrum's amplitudes carry the scale). n is a power of
	 * two.
	 */
	void OceanInverseFft(std::vector<std::complex<float>> &data);

	/** @brief x with its low bits (bits of them) in reverse order. */
	uint32_t OceanBitReverse(uint32_t x, uint32_t bits);

	/**
	 * @brief Distance (m) through the water at which the floor fades to turbidity/cutoff's transmittance (1%): the tile
	 * colour's alpha is the water's clarity (X-Plane's water maps: about 0.72 in Puget Sound, 0.75 in open water), so
	 * the distance is water/turbidity / (1 - alpha) (ASSUMPTION: the mapping isn't documented), at least clarityMinM.
	 */
	float WaterClarityDistance(float tileAlpha, const OceanSettings &settings);

	/** @brief Light left after pathM meters of water: exp(cutoff x path / clarity distance). 1 with turbidity off. */
	float WaterTransmittance(float pathM, float clarityDistanceM, const OceanSettings &settings);

	/** @brief How much of the waves' height a point of the water has: by the DSF fetch ratio (ponds calmer) and the floor depth (shores). */
	float WaterWaveScale(float fetchRatio, float floorDepthM, const OceanSettings &settings);

	/**
	 * @brief Tessellation factor of a water edge (water.slang's hull shader): its length over the length tessellationPixels
	 * pixels cover at its distance (pixelAngle: radians per pixel), 1..maxTessellation. From the edge alone, so the two
	 * triangles sharing it agree (no cracks).
	 */
	float WaterTessellationFactor(float edgeLengthM, float distanceM, float pixelAngle, const OceanSettings &settings);
}

// -------------------------------------------------------
