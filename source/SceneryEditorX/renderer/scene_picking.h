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
 * scene_picking.h
 * -------------------------------------------------------
 * Created: 19/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <vector>
#include <entt/entity/entity.hpp>
#include <entt/entity/fwd.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class CommandList;
	class ImageResource;

	/**
	 * @class ScenePicking
	 * @brief GPU picking and selection highlighting for the editor's viewport.
	 *
	 * The g-buffer pass writes a pick id per pixel into gbuffer_pick (R32_UINT): the drawing entity's index + 1, or 0
	 * where nothing was drawn. CPU draws carry it in DrawData::pick_id, GPU-scene instances in the upper bits of their
	 * flags. What a pixel shows is therefore exactly what can be clicked, including alpha-tested cut-outs, LODs and
	 * animated parts, at no CPU cost however large the scene.
	 *
	 * Reading it back is asynchronous: the editor requests a region (a few pixels around the cursor, or a marquee),
	 * the render thread copies it after the g-buffer pass into a host-visible buffer, and the result is available once
	 * that command list has completed (a couple of frames later). Requests are cheap: a 9 x 9 pixel copy.
	 *
	 * Highlighting: the editor hands over the pick ids of what is selected, active and hovered; a compute pass
	 * (selection_outline.slang) outlines those objects over the frame. Two bits per entity index live in a 256 KiB
	 * device buffer uploaded only when the highlight changes, and the pass is skipped while nothing is highlighted.
	 *
	 * Request / result / highlight calls come from the main thread; the Record* calls from the render thread.
	 */
	class ScenePicking
	{
	public:
		/* @brief Pick ids hold an entity index + 1 in 24 bits (they share a uint with flags on the GPU-scene path). */
		static constexpr uint32_t PICK_ID_MASK = 0x00FFFFFFu;

		/* @brief The pick id the passes write for an entity (0 for entt::null). */
		static uint32_t GetPickId(const entt::entity handle)
		{
			if (handle == entt::null)
				return 0u;
			return (static_cast<uint32_t>(entt::to_entity(handle)) + 1u) & PICK_ID_MASK;
		}

		/* @brief The live entity a pick id names (its index at the registry's current version), entt::null when none. */
		static entt::entity Resolve(const entt::registry &registry, uint32_t pickId);

		/* @brief A region of pick ids read back from the GPU. */
		struct Result
		{
			uint32_t x = 0;					// Region actually read, in render pixels (clamped to the target)
			uint32_t y = 0;
			uint32_t width = 0;
			uint32_t height = 0;
			std::vector<uint32_t> ids;		// width x height pick ids, row by row
		};

		/**
		 * @brief Queues a readback of the pick ids in a region of the render target.
		 * @param x, y Top-left corner in render pixels (0, 0 = the image's top-left).
		 * @return A ticket for TryGetResult(), 0 when the region is empty.
		 */
		static uint64_t Request(int32_t x, int32_t y, uint32_t width, uint32_t height);

		/* @brief Moves a finished request's result out. False while it is still in flight (or for an unknown ticket). */
		static bool TryGetResult(uint64_t ticket, Result &out);

		/* @brief Forgets a request (its result is dropped when it arrives). */
		static void Cancel(uint64_t ticket);

		/**
		 * @brief What the outline pass highlights, by pick id. Cheap to call every frame: nothing is uploaded unless
		 * the sets changed.
		 * @param selected Selected objects' pick ids.
		 * @param active The active (primary) selection's pick ids, drawn brighter.
		 * @param hovered The object under the cursor's pick ids.
		 */
		static void SetHighlight(const std::vector<uint32_t> &selected, const std::vector<uint32_t> &active, const std::vector<uint32_t> &hovered);

		/* @brief Turns the outline pass on or off (picking keeps working). */
		static void SetOutlineEnabled(bool enabled);
		static bool IsOutlineEnabled();

		/* @brief Render thread, after the g-buffer pass: reads back finished requests and copies the queued regions. */
		static void RecordReadbacks(CommandList *cmdList);

		/* @brief Render thread, over the composed frame (render resolution, before it is blitted to the output). */
		static void RecordOutline(CommandList *cmdList, ImageResource *target);

		/* @brief Leaves the pick target in the layout the g-buffer pass expects next frame. */
		static void EndFrame(CommandList *cmdList);

		/* @brief Releases the GPU buffers. Call before the device goes away. */
		static void Shutdown();
	};
}

// -------------------------------------------------------
