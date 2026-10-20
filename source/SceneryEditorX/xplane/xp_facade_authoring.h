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
 * xp_facade_authoring.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_assets.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * Making new facades: a type 1 building from one texture (its panels across and floors up marked on the texture),
	 * and type 2 ground facades - sidewalks, curbs, planters, low walls - generated from a cross-section, the way the
	 * X-Plane 12 library's 3D sidewalks are built (a walking-surface roof, curb segments in several lengths and curved
	 * variants cut into 0.5 - 0.8 m pieces). The results go to WriteFacadeAsset. Standard library only.
	 *
	 * Texture coordinates are normalized with t up from the bottom of the image (X-Plane's convention); a type 1 wall's
	 * textureWidth / textureHeight give the pixels the file is written in.
	 */

	// -------------------------------------------------------
	// Type 1 from a texture

	/* @brief A row of type 1 texture panels: across the wall (left / center / right) or up it (bottom / middle / top floors). */
	enum class FacadePanelRow : uint8_t
	{
		Left,
		Center,
		Right,
		Bottom,
		Middle,
		Top,
		Count
	};

	const char *GetFacadePanelRowName(FacadePanelRow row);
	inline bool IsFacadeFloorRow(const FacadePanelRow row) { return row >= FacadePanelRow::Bottom; }

	/**
	 * @brief A pixel position as the normalized coordinate the parser makes of it (pixels * (1 / size)), so it is written
	 * back as the same whole pixel. Set edges through this when the user types or snaps pixels. Itself when size is 0.
	 */
	inline float FacadeFromPixels(const float pixels, const float size) { return size > 0.0f ? pixels * (1.0f / size) : pixels; }

	std::vector<std::pair<float, float>> &GetFacadePanels(FacadeAsset::Wall &wall, FacadePanelRow row);
	const std::vector<std::pair<float, float>> &GetFacadePanels(const FacadeAsset::Wall &wall, FacadePanelRow row);

	/**
	 * @brief A new type 1 facade from one texture: a closed, draped building with one wall type whose SCALE makes the
	 * texture `metersAcross` wide (pixels square), the texture cut in quarters (left, two center quarters, right; bottom,
	 * two middle quarters, top) and the whole texture on the roof. Mark the real panels afterwards.
	 */
	FacadeAsset MakeType1Facade(const std::string &texture, int textureWidth, int textureHeight, float metersAcross);

	/* @brief A wall type like the facade's last one (or a fresh quarter-cut one), named "Wall N". */
	FacadeAsset::Wall MakeType1Wall(const FacadeAsset &asset);

	/* @brief Every panel edge on one axis of a wall (across: s, floors: t), sorted, without duplicates. */
	std::vector<float> GetFacadeTextureCuts(const FacadeAsset::Wall &wall, bool floors);

	/**
	 * @brief Moves a panel edge: every left / center / right (or bottom / middle / top) range ending or starting at
	 * `from` follows, so neighboring panels stay joined. Clamped to the texture and so no panel gets thinner than
	 * `minimum`. Returns where the edge ended up (`from` when no panel has it).
	 */
	float MoveFacadeTextureCut(FacadeAsset::Wall &wall, bool floors, float from, float to, float minimum = 1.0f / 4096.0f);

	/* @brief Splits a panel in two halves (both of the same row). */
	bool SplitFacadePanel(FacadeAsset::Wall &wall, FacadePanelRow row, size_t index);

	/* @brief Rounds a wall's panel edges (and basement) to whole pixels of its TEX_SIZE (nothing without one). */
	void SnapFacadeWallToPixels(FacadeAsset::Wall &wall);

	/* @brief Sets a type 1 facade's texture size on every wall and the roof (pixels; what coordinates are written in). */
	void SetFacadeTextureSize(FacadeAsset &asset, int width, int height);

	/* @brief A roof of four corners stretched over the roof (ROOF rows), counter-clockwise from s1 t1. */
	void SetFacadeRoofRectangle(FacadeAsset &asset, float s1, float t1, float s2, float t2);

	/* @brief A roof repeating a texture rectangle (type 1 ROOF_SCALE): the rectangle covers metersS x metersT of roof. */
	void SetFacadeRoofScaled(FacadeAsset &asset, float s1, float t1, float s2, float t2, float metersS, float metersT);

	/* @brief The meters a type 1 ROOF_SCALE rectangle covers (what SetFacadeRoofScaled took). */
	std::pair<float, float> GetFacadeRoofMeters(const FacadeAsset &asset);

	// -------------------------------------------------------
	// Type 2 sidewalks and curbs

	/* @brief A cross-section for a ground facade: points (outward, up) in meters from the walking surface's edge down to the ground. */
	struct CurbProfile
	{
		std::string name;
		std::vector<std::pair<float, float>> points;
	};

	/* @brief Ready-made cross-sections: X-Plane's 15 cm sidewalk curb, a low curb, a flush edge, a planter wall, a retaining wall. */
	const std::vector<CurbProfile> &GetCurbProfilePresets();

	struct CurbFacadeSpec
	{
		std::string wallName = "curb";		// The wall type's name (what the facade tool lists)
		std::vector<std::pair<float, float>> profile;	// (outward, up) meters; the roof is built at the first point's height
		std::string wallTexture;			// The curb's texture, relative to the .fac
		std::string wallTextureNormal;
		float textureS1 = 0.0f;				// Where the curb's strip is in the wall texture: s along the wall...
		float textureS2 = 1.0f;
		float textureT1 = 0.0f;				// ...t across the cross-section, t2 at its top edge and t1 at the ground
		float textureT2 = 1.0f;
		float textureLength = 1.0f;			// Meters of curb s1 - s2 covers (the strip repeats along the curb)
		std::string roofTexture;			// The walking surface, empty to use the curb's texture
		std::string roofTextureNormal;
		float roofScale = 8.0f;				// Meters per roof texture repeat
		std::string hardRoof = "concrete";	// Surface aircraft and vehicles feel on the roof, empty for none
		std::vector<float> segmentLengths = { 0.25f, 0.5f, 1.0f, 2.0f, 4.0f };	// One segment each
		float spellingLength = 8.0f;		// Spellings combine the segments into every multiple of the shortest up to this length
		float curvedPiece = 0.65f;			// Meters per piece of the SEGMENT_CURVED variants (the 3D sidewalk article's 0.5 - 0.8)
		float lodFar = 5000.0f;				// Meters the curb is drawn to
		bool ring = true;					// Closed (a sidewalk around an area) or an open run (a curb line)
	};

	/**
	 * @brief A type 2 ground facade from a cross-section: one floor whose roof (the walking surface) sits at the
	 * profile's top, one segment per length (the profile swept along the wall, the texture strip restarting every
	 * textureLength) with a SEGMENT_CURVED variant cut into curvedPiece pieces, and one wall type whose spellings fill
	 * every multiple of the shortest segment up to spellingLength (longest segments first), so X-Plane fits any wall
	 * within a fraction of the shortest segment.
	 */
	FacadeAsset BuildCurbFacade(const CurbFacadeSpec &spec);

	// -------------------------------------------------------
	// Files

	/* @brief Problems that keep a facade from working in X-Plane (no texture, a wall without panels, a spelling naming a missing segment...); empty when none. */
	std::vector<std::string> CheckFacadeAsset(const FacadeAsset &asset);

	/* @brief The pixel size of a PNG or DDS texture from its header; false when it can't be read. */
	bool ReadTextureSize(const std::filesystem::path &path, int &width, int &height);

	/* @brief A path as a facade writes it: relative to the folder of `fromFile` with forward slashes ("../textures/a.png"). */
	std::string MakeFacadeRelativePath(const std::filesystem::path &fromFile, const std::filesystem::path &target);

	/* @brief A file name from free text: letters, digits, '-' and '_' kept, the rest '_' ("Hangar 2 (red)" -> "Hangar_2_red_"). */
	std::string MakeFacadeFileName(const std::string &name);
}

// -------------------------------------------------------
