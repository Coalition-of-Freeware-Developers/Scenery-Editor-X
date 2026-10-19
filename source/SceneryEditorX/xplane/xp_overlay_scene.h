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
 * xp_overlay_scene.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include "xp_overlay.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

// -------------------------------------------------------

namespace SceneryEditorX::XPlane
{
	/**
	 * @class OverlayScene
	 * @brief Shows OverlayBuilder output in the active scene, under one root entity: an entity per group (a project item,
	 * say) drawing the group's meshes, with the group's objects (string objects, facade attachments, autogen) placed below
	 * it through the OBJ8 loader. Meshes become sub-meshes of one Mesh; textures load once per file and materials are
	 * shared per texture and alpha treatment. Main thread only.
	 */
	class OverlayScene
	{
	public:
		/* @brief What one entity shows: meshes and objects of a build, under a name and a key of the caller's. */
		struct Group
		{
			std::string key;
			std::string name;
			std::vector<const OverlayMesh *> meshes;
			std::vector<const OverlayObject *> objects;
		};

		struct Stats
		{
			uint32_t groups = 0;
			uint32_t parts = 0;					// Drawn sub-meshes (a group's meshes merged per material)
			uint32_t triangles = 0;
			uint32_t objects = 0;
			uint32_t missingTextures = 0;		// Parts drawn in their flat color because the texture couldn't be read
			double buildMilliseconds = 0.0;
		};

		OverlayScene();
		~OverlayScene();
		OverlayScene(const OverlayScene &) = delete;
		OverlayScene &operator=(const OverlayScene &) = delete;

		/**
		 * @brief Replaces what is shown. The root entity is created by the first build and kept by later ones (so
		 * selecting it survives rebuilds); a build without groups removes it.
		 * @return The root entity's UUID, 0 when nothing is shown or there is no active scene.
		 */
		uint64_t Build(const std::string &rootName, const std::vector<Group> &groups);

		/* @brief Removes the root and everything below it and releases the mesh, materials and textures. */
		void Clear();

		[[nodiscard]] uint64_t GetRootId() const;

		/* @brief The entity showing a group, 0 when none. */
		[[nodiscard]] uint64_t FindEntity(std::string_view key) const;

		/* @brief The key of the group an entity is part of (its entity, mesh entities, objects and their parts), empty when none. */
		[[nodiscard]] std::string FindKey(uint64_t entityId) const;

		/* @brief True for the root and every entity below it. */
		[[nodiscard]] bool Contains(uint64_t entityId) const;

		/* @brief Local-space footprint of everything drawn. */
		[[nodiscard]] Bounds2 GetBounds() const;

		[[nodiscard]] const Stats &GetStats() const;

	private:
		struct Impl;
		std::unique_ptr<Impl> m_Impl;
	};
}

// -------------------------------------------------------
