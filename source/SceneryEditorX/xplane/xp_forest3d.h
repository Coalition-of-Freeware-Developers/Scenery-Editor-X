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
 * xp_forest3d.h
 * -------------------------------------------------------
 * Created: 25/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_overlay.h"
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>
#include <entt/entt.hpp>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * @class Forest3d
	 * @brief X-Plane 12's 3D trees: the MESH a forest's TREE names with MESH_3D, drawn near the camera while the forest's
	 * billboards stand in farther out. The hand-over is a dithered per-pixel switch at GetSwitchDistance() (materials
	 * with XpLodSwitchMode: 1 the billboards, 2 the meshes; xp_material.slang xp_lod_switch_discard), so neither pops.
	 *
	 * Each tree kind of a planting draws from one entity: its trees within reach of the camera as instances of the kind's
	 * mesh, picked again when the camera has moved 32 m (Update()), so a forest costs a draw per kind and pass however
	 * far it spreads. Meshes, materials and textures are shared by every forest that names the same art. Main thread only.
	 */
	class Forest3d
	{
	public:
		/* @brief Meters from the camera where 3D trees hand over to billboards. */
		static float GetSwitchDistance();

		/**
		 * @brief True when every tree of the forest has a 3D mesh that loaded: its billboards may then hand over to 3D
		 * trees near the camera (forests with only some 3D trees keep their billboards everywhere and plant none).
		 */
		static bool DrawsEveryTree(const std::filesystem::path &forestFile);

		/**
		 * @brief Plants trees of a forest below parent (an entity per tree kind, made when Update() finds its first trees
		 * near the camera). Nothing when the forest doesn't DrawsEveryTree().
		 * @param trees Positions in parent's frame, y on the ground.
		 * @return Trees planted.
		 */
		static uint32_t Plant(entt::registry &registry, entt::entity parent, const std::filesystem::path &forestFile, const std::vector<OverlayTree3d> &trees,
			const std::string &name);

		/* @brief Gives the tree kinds their trees in reach of the render camera when it has moved far enough. Once per frame. */
		static void Update();

		/* @brief Trees planted (in forests still shown), and how many of them are drawn now. */
		static uint32_t GetTreeCount();
		static uint32_t GetDrawnTreeCount();

		/* @brief Removes the trees' entities and releases the meshes, materials and textures (editor shutdown). */
		static void Shutdown();
	};
}

// -------------------------------------------------------
