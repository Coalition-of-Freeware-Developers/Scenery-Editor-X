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
 * project_settings.h
 * -------------------------------------------------------
 * Created: 15/6/2025
 * -------------------------------------------------------
 */
#pragma once
#include <string>
#include <string_view>

// -------------------------------------------------------

namespace SceneryEditorX
{
    struct ProjectConfig
	{
        std::string name;
		std::string projectName;
		std::string projectPath;
        std::string assetRegistry;
		std::string sceneryType;
        std::string sceneryVersion;
        std::string assetDir = "Assets";

		bool enableAutosave = true;
		bool enableLighting = true;
		bool enableWeatherEffects = false;
        int autosaveInterval = 300; // 5 mins in seconds
    };

	struct RecentProject
	{
		std::string name;
		std::string filePath;
		time_t lastOpened;
	};

	// -------------------------------------------------------

	namespace Tiering::Renderer
	{

		enum class ShadowQualitySetting : uint8_t
		{
		    None	= BIT(0),
		    Low		= BIT(1),
		    High	= BIT(2)
		};

		enum class ShadowResolutionSetting : uint8_t
		{
		    None	= BIT(0),
		    Low		= BIT(1),
		    Medium	= BIT(2),
		    High	= BIT(3)
		};

		enum class AmbientOcclusionTypeSetting : uint8_t
		{
		    None	= BIT(0),
		    GTAO	= BIT(1)
		};

		enum class AmbientOcclusionQualitySetting : uint8_t
		{
		    None	= BIT(0),
		    High	= BIT(1),
		    Ultra	= BIT(2)
		};

		enum class SSRQualitySetting : uint8_t
		{
		    Off		= BIT(0),
		    Medium	= BIT(1),
		    High	= BIT(2)
		};

		struct RendererTieringSettings
		{
		    float rendererScale = 1.0f;
		    bool windowed		= false;
		    bool vSync			= true;
		    bool enableBloom	= true;

		    // Shadows
		    bool enableShadows = true;
		    ShadowQualitySetting shadowQuality = ShadowQualitySetting::High;
		    ShadowResolutionSetting shadowResolution = ShadowResolutionSetting::High;

		    // Ambient Occlusion
		    bool enableAO = true;
		    AmbientOcclusionTypeSetting type = AmbientOcclusionTypeSetting::GTAO;
		    AmbientOcclusionQualitySetting quality = AmbientOcclusionQualitySetting::Ultra;
		    SSRQualitySetting SSRQuality = SSRQualitySetting::Off;
		};

		inline const char* ShadowQualitySettingToString(const ShadowQualitySetting shadowQualitySetting)
		{
		    switch (shadowQualitySetting)
		    {
				case ShadowQualitySetting::None: return "None";
				case ShadowQualitySetting::Low:  return "Low";
				case ShadowQualitySetting::High: return "High";
		    }

		    return nullptr;
		}

		inline ShadowQualitySetting ShadowQualitySettingFromString(const std::string_view setting)
		{
		    if (setting == "None") return ShadowQualitySetting::None;
		    if (setting == "Low")  return ShadowQualitySetting::Low;
		    if (setting == "High") return ShadowQualitySetting::High;

		    return ShadowQualitySetting::None;
		}

		inline const char* ShadowResolutionSettingToString(const ShadowResolutionSetting shadowResolutionSetting)
		{
		    switch (shadowResolutionSetting)
		    {
				case ShadowResolutionSetting::None:   return "None";
				case ShadowResolutionSetting::Low:    return "Low";
				case ShadowResolutionSetting::Medium: return "Medium";
				case ShadowResolutionSetting::High:   return "High";
		    }

		    return nullptr;
		}

		inline ShadowResolutionSetting ShadowResolutionSettingFromString(const std::string_view setting)
		{
		    if (setting == "None")   return ShadowResolutionSetting::None;
		    if (setting == "Low")    return ShadowResolutionSetting::Low;
		    if (setting == "Medium") return ShadowResolutionSetting::Medium;
		    if (setting == "High")   return ShadowResolutionSetting::High;

		    return ShadowResolutionSetting::None;
		}

		inline const char* AmbientOcclusionQualitySettingToString(const AmbientOcclusionQualitySetting ambientOcclusionQualitySetting)
		{
		    switch (ambientOcclusionQualitySetting)
		    {
				case AmbientOcclusionQualitySetting::None:   return "None";
				case AmbientOcclusionQualitySetting::High:   return "High";
				case AmbientOcclusionQualitySetting::Ultra:  return "Ultra";
		    }

		    return nullptr;
		}

		inline AmbientOcclusionQualitySetting AmbientOcclusionQualitySettingFromString(const std::string_view setting)
		{
		    if (setting == "None")   return AmbientOcclusionQualitySetting::None;
		    if (setting == "Low")    return AmbientOcclusionQualitySetting::High; /// NOTE: low has been renamed to high, currently there is no low.
		    if (setting == "High")   return AmbientOcclusionQualitySetting::High;
		    if (setting == "Ultra")  return AmbientOcclusionQualitySetting::Ultra;

		    return AmbientOcclusionQualitySetting::None;
		}

		inline const char* SSRQualitySettingToString(const SSRQualitySetting ssrQualitySetting)
		{
		    switch (ssrQualitySetting)
		    {
		    case SSRQualitySetting::Off:    return "Off";
		    case SSRQualitySetting::Medium: return "Medium";
		    case SSRQualitySetting::High:   return "High";
		    }

		    return nullptr;
		}

		inline SSRQualitySetting SSRQualitySettingFromString(const std::string_view ssrQualitySetting)
		{
		    if (ssrQualitySetting == "Off")    return SSRQualitySetting::Off;
		    if (ssrQualitySetting == "Medium") return SSRQualitySetting::Medium;
		    if (ssrQualitySetting == "High")   return SSRQualitySetting::High;

		    return SSRQualitySetting::Off;
		}

	}

	// -------------------------------------------------------

    namespace Tiering
	{
		struct TieringSettings
		{
		    Renderer::RendererTieringSettings RendererTS;
		};
	}

}

// -------------------------------------------------------
