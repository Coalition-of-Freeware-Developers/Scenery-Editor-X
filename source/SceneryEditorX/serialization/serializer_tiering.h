/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* serializer_tiering.h
* -------------------------------------------------------
* Created: 11/7/2025
* -------------------------------------------------------
*/
#pragma once
#include <filesystem>
#include <SceneryEditorX/project/project_settings.h>

/// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class TieringSerializer
	 * @brief Handles serialization/deserialization of rendering quality settings.
	 *
	 * The TieringSerializer provides static methods to save and load renderer tiering
	 * settings to/from libconfig (.cfg) format files. This includes shadow quality,
	 * ambient occlusion settings, screen-space reflections, and other rendering
	 * quality parameters.
	 *
	 * Features:
	 * - Structured hierarchical configuration format
	 * - Legacy format compatibility for smooth upgrades
	 * - Comprehensive error handling and logging
	 * - Automatic directory creation for output files
	 */
	class TieringSerializer
	{
	public:
		/**
		 * @brief Serializes tiering settings to a .cfg file.
		 *
		 * Saves the provided TieringSettings structure to a structured libconfig
		 * format file. Creates necessary directories if they don't exist.
		 *
		 * @param tieringSettings The settings to serialize
		 * @param filepath Path to the output .cfg file
		 */
		static void Serialize(const Tiering::TieringSettings& tieringSettings, const std::filesystem::path& filepath);

		/**
		 * @brief Deserializes tiering settings from a .cfg file.
		 *
		 * Loads TieringSettings from a libconfig format file. Provides fallback
		 * values for missing settings and supports legacy format compatibility.
		 *
		 * @param outTieringSettings Reference to populate with loaded settings
		 * @param filepath Path to the input .cfg file
		 * @return true if successful, false if file doesn't exist or parsing failed
		 */
		static bool Deserialize(Tiering::TieringSettings& outTieringSettings, const std::filesystem::path& filepath);
	};

}

/// -------------------------------------------------------
