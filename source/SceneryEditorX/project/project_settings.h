/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* project_settings.h
* -------------------------------------------------------
* Created: 15/6/2025
* -------------------------------------------------------
*/
#pragma once
#include <string>
#include <string_view>

/// -------------------------------------------------------

namespace SceneryEditorX
{
    struct ProjectConfig
	{
        std::string name;
		std::string projectName;
		std::string projectPath;
		std::string sceneryType;
        std::string sceneryVersion;
        std::string assetDir = "Assets";

		bool enableAutosave = true;
		bool enableLighting = true;
		bool enableWeatherEffects = false;
        int autosaveInterval = 300; /// 5 mins in seconds
    };

	struct RecentProject
	{
		std::string name;
		std::string filePath;
		time_t lastOpened;
	};

	/// -------------------------------------------------------

	namespace Tiering
    {
		namespace Renderer
	    {
			enum class ShadowQualitySetting : uint8_t
			{
				None = 0,
			    Low = 1,
			    High = 2
			};

			enum class ShadowResolutionSetting : uint8_t
			{
				None = 0,
			    Low = 1,
			    Medium = 2,
			    High = 3
			};

			enum class AmbientOcclusionTypeSetting : uint8_t
			{
				None = 0,
			    GTAO = 1
			};

			enum class AmbientOcclusionQualitySetting : uint8_t
			{
				None = 0,
			    High = 1,
			    Ultra = 2
			};

			enum class SSRQualitySetting : uint8_t
			{
				Off = 0,
			    Medium = 1,
			    High = 2
			};

			struct RendererTieringSettings
			{
				float RendererScale = 1.0f;
				bool Windowed = false;
				bool VSync = true;

				///< Shadows
				bool EnableShadows = true;
				ShadowQualitySetting ShadowQuality = ShadowQualitySetting::High;
				ShadowResolutionSetting ShadowResolution = ShadowResolutionSetting::High;

				///< Ambient Occlusion
				bool EnableAO = true;
				AmbientOcclusionTypeSetting AOType = AmbientOcclusionTypeSetting::GTAO;
				AmbientOcclusionQualitySetting AOQuality = AmbientOcclusionQualitySetting::Ultra;
				SSRQualitySetting SSRQuality = SSRQualitySetting::Off;

				bool EnableBloom = true;
			};

			namespace Utils
		    {

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

		}

	    /// -------------------------------------------------------

		struct TieringSettings
		{
			Renderer::RendererTieringSettings RendererTS;
		};

	    /// -------------------------------------------------------

	}

}

/// -------------------------------------------------------
