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
 * renderer_weather.cpp
 * -------------------------------------------------------
 * Created: 25/10/2026
 * -------------------------------------------------------
 *
 * Weather on surfaces of the XP12 emulation (plan Phase 12; XP12: weather_apply). The environment's surface weather
 * (wet, puddles, snow, ice) becomes the thresholds weather.slang compares every surface's receptivity with, and the
 * twxr controls its look. X-Plane's weather art rides in a hidden material so the shaders reach it through the bindless
 * material table (UpdateMaterials adds it after the scene's materials) instead of new descriptor bindings.
 */
#include "renderer.h"
#include <SceneryEditorX/environment/environment.h>
#include <SceneryEditorX/scene/material.h>
#include <algorithm>
#include <utility>

// -------------------------------------------------------

namespace SceneryEditorX
{
	namespace
	{
		// A threshold no receptivity reaches (receptivities are 0-1, the noise moves them by a fraction)
		constexpr float NO_WEATHER = 4.0f;

		Renderer::WeatherArt s_WeatherArt;
		Ref<MaterialAsset> s_WeatherMaterial;
		bool s_WeatherArtChanged = false;

		float PerMeter(const float meters)
		{
			return 1.0f / std::max(meters, 1e-3f);
		}
	}

	void Renderer::SetWeatherArt(const WeatherArt &art)
	{
		s_WeatherArt = art;

		// Texture slots (weather.slang weather_* indices): albedo 0 snow, 1 ice, 2 snow decal, 3 ice decal; normal 0 snow,
		// 1 ice; roughness 0 the noise
		Ref<MaterialAsset> material = CreateRef<MaterialAsset>();
		material->SetName("X-Plane weather art");
		material->SetTexture(MaterialTextureType::Color, art.snowAlbedo.Get(), 0);
		material->SetTexture(MaterialTextureType::Color, art.iceAlbedo.Get(), 1);
		material->SetTexture(MaterialTextureType::Color, art.snowDecal.Get(), 2);
		material->SetTexture(MaterialTextureType::Color, art.iceDecal.Get(), 3);
		material->SetTexture(MaterialTextureType::Normal, art.snowNormal.Get(), 0);
		material->SetTexture(MaterialTextureType::Normal, art.iceNormal.Get(), 1);
		material->SetTexture(MaterialTextureType::Roughness, art.noise.Get(), 0);
		s_WeatherMaterial = material;
		s_WeatherArtChanged = true;
	}

	bool Renderer::HasWeatherArt()
	{
		return s_WeatherArt.snowAlbedo && s_WeatherArt.snowNormal && s_WeatherArt.snowDecal && s_WeatherArt.iceAlbedo &&
			   s_WeatherArt.iceNormal && s_WeatherArt.iceDecal && s_WeatherArt.noise;
	}

	MaterialAsset *Renderer::GetWeatherMaterial()
	{
		return s_WeatherMaterial.Get();
	}

	bool Renderer::ConsumeWeatherArtChange()
	{
		return std::exchange(s_WeatherArtChanged, false);
	}

	void Renderer::ReleaseWeatherArt()
	{
		s_WeatherMaterial = nullptr;
		s_WeatherArt = {};
	}

	void Renderer::WriteWeatherConstants(ConstantBuffer_Environment &env)
	{
		const WeatherSurfaceSettings &settings = GetEnvironmentConstants().weather;
		const Environment::SurfaceWeather surface = Environment::Get().GetSnapshot().derived.surface;

		// The thresholds, in X-Plane's order (rain, ice, snow, puddles); off, nothing passes
		const bool on = GetFeatures().weatherApply;
		const auto threshold = [on](const Environment::WeatherThresholdCurve &curve, const float amount)
		{
			return on && amount > 0.0f ? curve.Evaluate(std::clamp(amount, 0.0f, 1.0f)) : NO_WEATHER;
		};
		env.weather = xMath::Vec4(threshold(settings.rainThresholds, surface.wetness), threshold(settings.iceThresholds, surface.ice),
								  threshold(settings.snowThresholds, surface.snow), threshold(settings.puddleThresholds, surface.puddles));

		env.weather_rain = xMath::Vec4(PerMeter(settings.rainAreaScaleM), settings.rainAreaWidth, settings.rainNoiseModifier, settings.darkeningScale);
		env.weather_darkening = xMath::Vec4(settings.darkeningStart, settings.darkeningEnd, settings.darkeningStrength, settings.darkenAlbedoRatio);
		env.weather_darkening_gloss = xMath::Vec4(settings.darkeningRoughnessStart, settings.darkeningRoughnessEnd, settings.roughnessMultiplierDark,
												  settings.roughnessMultiplierLight);
		env.weather_water = xMath::Vec4(settings.darkenGlossRatio, settings.waterGloss, 0.0f, 0.0f);
		env.weather_snow = xMath::Vec4(PerMeter(settings.snowAreaScaleM), settings.snowAreaWidth, settings.snowNoiseDepth, 0.0f);
		env.weather_snow_detail = xMath::Vec4(settings.snowSlopeMin, settings.snowSlopeMax, settings.snowDecal1K, settings.snowDecal2K);
		env.weather_snow_scale = xMath::Vec4(PerMeter(settings.snowAlbedoM), PerMeter(settings.snowNormalM), PerMeter(settings.snowDecalM), 0.0f);
		env.weather_ice = xMath::Vec4(PerMeter(settings.iceAreaScaleM), settings.iceAreaWidth, settings.iceDecal1K, settings.iceDecal2K);
		env.weather_ice_scale = xMath::Vec4(PerMeter(settings.iceAlbedoM), PerMeter(settings.iceNormalM), PerMeter(settings.iceDecalM), 0.0f);
		env.weather_snow_luma = xMath::Vec4(settings.snowLuma[0], settings.snowLuma[1], settings.snowLuma[2], settings.snowLuma[3]);

		// The art's material slot; UpdateMaterials placed it after the scene's materials this frame or earlier
		const MaterialAsset *material = GetWeatherMaterial();
		const bool art = material && HasWeatherArt();
		env.weather_textures = xMath::Vec4(art ? static_cast<float>(material->GetIndex()) : 0.0f, art ? 1.0f : 0.0f, 0.0f, 0.0f);
	}
}

// -------------------------------------------------------
