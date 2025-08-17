/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* serializer_tiering.cpp
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#include <libconfig.h++>
#include "serializer_tiering.h"

/// -------------------------------------------------------

namespace SceneryEditorX
{

    using namespace libconfig;

    /**
     * @brief Creates directories if they don't exist for the given file path.
     * @param path The file path whose parent directories should be created
     */
    static void CreateDirectoriesIfNeeded(const std::filesystem::path& path)
    {
        if (const std::filesystem::path directory = path.parent_path(); !directory.empty() && !std::filesystem::exists(directory))
        {
            std::filesystem::create_directories(directory);
            SEDX_CORE_TRACE_TAG("TIERING", "Created directory: {}", directory.string());
        }
    }

    /**
     * @brief Serializes tiering settings to a libconfig (.cfg) file.
     *
     * This function converts the TieringSettings structure to a structured
     * configuration file using libconfig format. The settings include renderer
     * quality options such as shadow quality, ambient occlusion, and SSR settings.
     *
     * @param tieringSettings The tiering settings to serialize
     * @param filepath Path to the .cfg file to write
     */
    void TieringSerializer::Serialize(const Tiering::TieringSettings& tieringSettings, const std::filesystem::path& filepath)
    {
        using namespace Tiering::Renderer;

        try
        {
            Config cfg;
            cfg.setOptions(Config::OptionAutoConvert | Config::OptionOpenBraceOnSeparateLine | Config::OptionFsync);
            cfg.setTabWidth(2);

            ///< Create root group for tiering settings
            Setting& root = cfg.getRoot();
            Setting& tieringGroup = root.add("TieringSettings", Setting::TypeGroup);

            ///< Renderer settings sub-group
            Setting& rendererGroup = tieringGroup.add("Renderer", Setting::TypeGroup);

            ///< Basic renderer settings
            rendererGroup.add("RendererScale", Setting::TypeFloat) = tieringSettings.RendererTS.RendererScale;
            rendererGroup.add("Windowed", Setting::TypeBoolean) = tieringSettings.RendererTS.Windowed;
            rendererGroup.add("VSync", Setting::TypeBoolean) = tieringSettings.RendererTS.VSync;

            ///< Shadow settings subgroup
            Setting& shadowGroup = rendererGroup.add("Shadows", Setting::TypeGroup);
            shadowGroup.add("EnableShadows", Setting::TypeBoolean) = tieringSettings.RendererTS.EnableShadows;
            shadowGroup.add("Quality", Setting::TypeString) = ShadowQualitySettingToString(tieringSettings.RendererTS.ShadowQuality);
            shadowGroup.add("Resolution", Setting::TypeString) = ShadowResolutionSettingToString(tieringSettings.RendererTS.ShadowResolution);

            ///< Ambient Occlusion settings subgroup
            Setting& aoGroup = rendererGroup.add("AmbientOcclusion", Setting::TypeGroup);
            aoGroup.add("EnableAO", Setting::TypeBoolean) = tieringSettings.RendererTS.EnableAO;
            aoGroup.add("Type", Setting::TypeString) = (tieringSettings.RendererTS.AOType == AmbientOcclusionTypeSetting::GTAO) ? "GTAO" : "None";
            aoGroup.add("Quality", Setting::TypeString) = AmbientOcclusionQualitySettingToString(tieringSettings.RendererTS.AOQuality);

            ///< Screen Space Reflections settings
            rendererGroup.add("SSRQuality", Setting::TypeString) = SSRQualitySettingToString(tieringSettings.RendererTS.SSRQuality);

            ///< Post-processing effects
            rendererGroup.add("EnableBloom", Setting::TypeBoolean) = tieringSettings.RendererTS.EnableBloom;

            ///< Ensure directory exists and write file
            CreateDirectoriesIfNeeded(filepath);
            cfg.writeFile(filepath.string().c_str());

            SEDX_CORE_INFO_TAG("TIERING", "Tiering settings serialized to: {}", filepath.string());
        }
        catch (const ConfigException& ex)
        {
            SEDX_CORE_ERROR_TAG("TIERING", "Failed to serialize tiering settings: {}", ex.what());
        }
        catch (const std::exception& ex)
        {
            SEDX_CORE_ERROR_TAG("TIERING", "Exception during tiering serialization: {}", ex.what());
        }
    }

    /**
     * @brief Deserializes tiering settings from a libconfig (.cfg) file.
     *
     * This function reads a structured configuration file and populates the
     * TieringSettings structure with the loaded values. It provides fallback
     * to default values if settings are missing and supports legacy format
     * compatibility.
     *
     * @param outTieringSettings Reference to TieringSettings to populate
     * @param filepath Path to the .cfg file to read
     * @return true if deserialization was successful, false otherwise
     */
    bool TieringSerializer::Deserialize(Tiering::TieringSettings& outTieringSettings, const std::filesystem::path& filepath)
    {
        using namespace Tiering::Renderer;

        try
        {
            if (!std::filesystem::exists(filepath))
            {
                SEDX_CORE_WARN_TAG("TIERING", "Tiering settings file not found: {}", filepath.string());
                return false;
            }

            Config cfg;
            cfg.readFile(filepath.string().c_str());

            ///< Check if TieringSettings group exists
            if (!cfg.exists("TieringSettings"))
            {
                SEDX_CORE_ERROR_TAG("TIERING", "TieringSettings group not found in config file");
                return false;
            }

            const Setting& tieringSettings = cfg.lookup("TieringSettings");

            ///< Read renderer settings if they exist
            if (tieringSettings.exists("Renderer"))
            {
                const Setting& renderer = tieringSettings["Renderer"];

                ///< Basic renderer settings with defaults
                if (renderer.exists("RendererScale"))
                    outTieringSettings.RendererTS.RendererScale = static_cast<float>(renderer["RendererScale"]);

                if (renderer.exists("Windowed"))
                    outTieringSettings.RendererTS.Windowed = static_cast<bool>(renderer["Windowed"]);

                if (renderer.exists("VSync"))
                    outTieringSettings.RendererTS.VSync = static_cast<bool>(renderer["VSync"]);

                ///< Shadow settings
                if (renderer.exists("Shadows"))
                {
                    const Setting& shadows = renderer["Shadows"];

                    if (shadows.exists("EnableShadows"))
                        outTieringSettings.RendererTS.EnableShadows = static_cast<bool>(shadows["EnableShadows"]);

                    if (shadows.exists("Quality"))
                    {
                        std::string qualityStr = static_cast<const char*>(shadows["Quality"]);
                        outTieringSettings.RendererTS.ShadowQuality = ShadowQualitySettingFromString(qualityStr);
                    }

                    if (shadows.exists("Resolution"))
                    {
                        std::string resolutionStr = static_cast<const char*>(shadows["Resolution"]);
                        outTieringSettings.RendererTS.ShadowResolution = ShadowResolutionSettingFromString(resolutionStr);
                    }
                }

                ///< Ambient Occlusion settings
                if (renderer.exists("AmbientOcclusion"))
                {
                    const Setting& ao = renderer["AmbientOcclusion"];

                    if (ao.exists("EnableAO"))
                        outTieringSettings.RendererTS.EnableAO = static_cast<bool>(ao["EnableAO"]);

                    if (ao.exists("Type"))
                    {
                        std::string typeStr = static_cast<const char*>(ao["Type"]);
                        outTieringSettings.RendererTS.AOType = (typeStr == "GTAO") ?
                            AmbientOcclusionTypeSetting::GTAO : AmbientOcclusionTypeSetting::None;
                    }

                    if (ao.exists("Quality"))
                    {
                        std::string qualityStr = static_cast<const char*>(ao["Quality"]);
                        outTieringSettings.RendererTS.AOQuality = AmbientOcclusionQualitySettingFromString(qualityStr);
                    }
                }

                ///< Screen Space Reflections
                if (renderer.exists("SSRQuality"))
                {
                    std::string ssrStr = static_cast<const char*>(renderer["SSRQuality"]);
                    outTieringSettings.RendererTS.SSRQuality = SSRQualitySettingFromString(ssrStr);
                }

                ///< Post-processing effects
                if (renderer.exists("EnableBloom"))
                    outTieringSettings.RendererTS.EnableBloom = static_cast<bool>(renderer["EnableBloom"]);
            }

            ///< Legacy compatibility: Check for old flat structure
            if (tieringSettings.exists("RendererScale"))
                outTieringSettings.RendererTS.RendererScale = static_cast<float>(tieringSettings["RendererScale"]);

            if (tieringSettings.exists("Windowed"))
                outTieringSettings.RendererTS.Windowed = static_cast<bool>(tieringSettings["Windowed"]);

            if (tieringSettings.exists("VSync"))
                outTieringSettings.RendererTS.VSync = static_cast<bool>(tieringSettings["VSync"]);

            ///< Legacy shadow settings
            if (tieringSettings.exists("ShadowQuality"))
            {
                std::string qualityStr = static_cast<const char*>(tieringSettings["ShadowQuality"]);
                outTieringSettings.RendererTS.ShadowQuality = ShadowQualitySettingFromString(qualityStr);
            }

            if (tieringSettings.exists("ShadowResolution"))
            {
                std::string resolutionStr = static_cast<const char*>(tieringSettings["ShadowResolution"]);
                outTieringSettings.RendererTS.ShadowResolution = ShadowResolutionSettingFromString(resolutionStr);
            }

            ///< Legacy AO settings (old format support)
            if (tieringSettings.exists("AmbientOcclusionQuality"))
            {
                std::string aoQualityStr = static_cast<const char*>(tieringSettings["AmbientOcclusionQuality"]);
                outTieringSettings.RendererTS.AOQuality = AmbientOcclusionQualitySettingFromString(aoQualityStr);
            }
            else if (tieringSettings.exists("AmbientOcclusion"))
            {
                ///< Very old format compatibility
                bool enableAO = static_cast<bool>(tieringSettings["AmbientOcclusion"]);
                outTieringSettings.RendererTS.AOQuality = enableAO ?
                    AmbientOcclusionQualitySetting::High : AmbientOcclusionQualitySetting::None;
                outTieringSettings.RendererTS.EnableAO = enableAO;
            }

            if (tieringSettings.exists("SSRQuality"))
            {
                std::string ssrStr = static_cast<const char*>(tieringSettings["SSRQuality"]);
                outTieringSettings.RendererTS.SSRQuality = SSRQualitySettingFromString(ssrStr);
            }

            SEDX_CORE_INFO_TAG("TIERING", "Tiering settings loaded from: {}", filepath.string());
            return true;
        }
        catch (const FileIOException& ex)
        {
            SEDX_CORE_ERROR_TAG("TIERING", "File I/O error reading tiering settings: {}", ex.what());
            return false;
        }
        catch (const ParseException& ex)
        {
            SEDX_CORE_ERROR_TAG("TIERING", "Parse error in tiering config at line {}: {}", ex.getLine(), ex.getError());
            return false;
        }
        catch (const ConfigException& ex)
        {
            SEDX_CORE_ERROR_TAG("TIERING", "Config error reading tiering settings: {}", ex.what());
            return false;
        }
        catch (const std::exception& ex)
        {
            SEDX_CORE_ERROR_TAG("TIERING", "Exception during tiering deserialization: {}", ex.what());
            return false;
        }
    }

}

/// -------------------------------------------------------
