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
 * serializer_tiering.h
 * -------------------------------------------------------
 * Created: 11/7/2025
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include "SceneryEditorX/project/project_settings.h"

// -------------------------------------------------------

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

// -------------------------------------------------------
