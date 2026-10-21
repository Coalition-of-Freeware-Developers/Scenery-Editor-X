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
 * asset_thumbnails.h
 * -------------------------------------------------------
 * Created: 21/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <filesystem>
#include <string>
#include <SceneryEditorX/renderer/ui/source/imgui/imgui.h>

// -------------------------------------------------------

namespace SceneryEditorX::AssetThumbnails
{
	/**
	 * Preview images of asset files for the X-PLANE LIBRARY and ASSETS panels (XPlane::ThumbnailBuilder draws them the
	 * way WorldEditor previews library items).
	 *
	 * A thumbnail is asked for while a panel draws (Get / Draw): the first ask queues it, two background workers read it
	 * from the disk cache (<cache>/thumbnails/<hash of the file's path, size, time and the thumbnail version>.png) or
	 * render it and write it there, and Update() uploads the finished ones on the main thread under a time budget.
	 * Small thumbnails (128 px) live in a few 1024 px sRGB atlas pages, the least recently drawn making room; large ones
	 * (256 px, the details pane) are separate images, the last few kept. Requests nobody has drawn for a while are
	 * dropped from the queue. Main thread only, apart from the workers.
	 */

	enum class Size : uint8_t
	{
		Small,		// 128 px: list rows, grid tiles
		Large		// 256 px: the details pane
	};

	enum class State : uint8_t
	{
		Unsupported,	// No thumbnail for this kind of file
		Pending,		// Queued or rendering
		Ready,
		Failed			// GetError() says why
	};

	struct Thumbnail
	{
		ImTextureID texture = ImTextureID_Invalid;	// Valid for this UI frame
		ImVec2 uv0{ 0.0f, 0.0f };
		ImVec2 uv1{ 1.0f, 1.0f };
	};

	/* @brief The thumbnail of a file, queued the first time it is asked for. */
	State Get(const std::filesystem::path &file, Thumbnail &out, Size size = Size::Small);

	/**
	 * @brief Draws a file's thumbnail fitted into a rectangle of the current window's draw list (nothing while it renders).
	 * @return True when it was drawn; otherwise the caller draws its usual icon.
	 */
	bool Draw(const std::filesystem::path &file, const ImVec2 &min, const ImVec2 &max, Size size = Size::Small);

	/* @brief Why a file's thumbnail failed (empty when it didn't). */
	std::string GetError(const std::filesystem::path &file, Size size = Size::Small);

	/* @brief Draws a file's thumbnail again next time it is asked for (the file changed). */
	void Invalidate(const std::filesystem::path &file);

	/* @brief Once per frame on the main thread: drops stale requests, uploads finished thumbnails, starts workers. */
	void Update();

	/* @brief Waits for the workers and frees the GPU images (before the renderer shuts down). */
	void Shutdown();

	struct Stats
	{
		size_t ready = 0;			// Thumbnails on the GPU
		size_t pending = 0;			// Queued or rendering
		size_t failed = 0;
		uint64_t rendered = 0;		// Drawn by the renderer since startup
		uint64_t fromDisk = 0;		// Read from the disk cache since startup
		double renderMilliseconds = 0.0;	// Total time the workers spent rendering
	};
	Stats GetStats();

}

// -------------------------------------------------------
