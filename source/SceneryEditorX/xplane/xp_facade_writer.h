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
 * xp_facade_writer.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_assets.h"
#include <filesystem>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Writes facades (.fac) back out, per https://developer.x-plane.com/article/x-plane-10-facade-fac-file-format-specification/.
	 * Standard library only.
	 *
	 * Type 1 (version 800): the facade-wide switches, the shader sections, one LOD (0 to lodFar) holding the roof (ROOF
	 * corners or ROOF_SCALE) and every WALL with its WALL_RULEs, SCALE, ROOF_SLOPE, BASEMENT_DEPTH and BOTTOM / MIDDLE /
	 * TOP / LEFT / CENTER / RIGHT rows. Texture coordinates are written in each wall's (and the roof's) TEX_SIZE pixels
	 * when it has one, else as fractions.
	 *
	 * Type 2 (version 1000): the switches, SHADER_WALL / SHADER_ROOF (TEXTURE, TEXTURE_NORMAL, NO_BLEND, TWO_SIDED and the
	 * rows kept verbatim), ROOF_SCALE, OBJ, then each FLOOR with its ROOF_HEIGHTs (TWO_SIDED_ROOF, ROOF_OBJ(_HEADING)),
	 * HARD_ROOF, SEGMENTs and SEGMENT_CURVEDs (MESH / VERTEX / IDX, ATTACH_DRAPED / ATTACH_GRADED) and WALLs (WALL_RULE,
	 * SPELLING).
	 *
	 * What ParseFacadeAsset reads is what is written: parsing the output gives the same FacadeAsset (numbers are written
	 * so they read back to the same floats), and writing that again gives the same text. Far LODs aren't kept by the
	 * parser, so a rewritten type 1 facade has only its nearest LOD.
	 */

	/* @brief The facade as .fac text (LF line endings). */
	std::string WriteFacadeAsset(const FacadeAsset &asset);

	/* @brief Writes a .fac file atomically (beside the target, then renamed over it). */
	bool SaveFacadeAsset(const std::filesystem::path &path, const FacadeAsset &asset, std::string *error = nullptr);

	/* @brief A float in its shortest form that reads back exactly ("0.15", "1024", "-0.0005"). */
	std::string FormatFacadeNumber(float value);
	void AppendFacadeNumber(std::string &out, float value);
}

// -------------------------------------------------------
