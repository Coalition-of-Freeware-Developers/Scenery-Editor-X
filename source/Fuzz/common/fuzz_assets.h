/**
 * -------------------------------------------------------
 * Scenery Editor X - Fuzzing
 * -------------------------------------------------------
 * Copyright (c) 2026 Thomas Ray
 * Copyright (c) 2026 Coalition of Freeware Developers
 * -------------------------------------------------------
 * fuzz_assets.h
 * -------------------------------------------------------
 * A small scenery pack in the temp folder with one asset of every kind the overlay builders draw (.pol, .lin, .str,
 * .for, two .fac, .agp, .net, .obj). Harnesses that fuzz placements (DSF tiles, apt.dat surfaces, projects) resolve
 * every asset path to it, so fuzzed geometry reaches the triangulation, ribbon, forest, facade and road builders
 * instead of stopping at "asset not found".
 * -------------------------------------------------------
 */
#pragma once

#include <SceneryEditorX/xplane/xp_overlay.h>
#include <filesystem>

namespace SceneryEditorX::Fuzz
{
	/* @brief The pack folder, written once per process (removed at exit). */
	const std::filesystem::path &GetAssetPack();

	/**
	 * @brief Resolves any asset path to the pack's asset of the same kind (by extension; the extension-less library
	 * prefixes for apt.dat lines and light strings to the .lin / .str). Unknown kinds resolve to nothing.
	 */
	XPlane::AssetResolver MakeAssetResolver();

	/* @brief Checks what the scene uploads: every mesh's indices are whole triangles inside its vertices. */
	void CheckOverlay(const XPlane::OverlayBuild &build);

	/**
	 * @brief Writes the input as an asset of the pack ("fuzzed" + extension) and draws it through the overlay builder
	 * the way a DSF tile places it: polygon assets (.pol .lin .str .for .fac) on a square, a hole, an open line, wall
	 * types and bezier curves with assorted parameters; .agp as an object; .net as a curved road segment.
	 */
	void BuildPlacedAsset(std::string_view extension, const uint8_t *data, size_t size);
}
