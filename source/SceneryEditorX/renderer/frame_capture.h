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
 * frame_capture.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <filesystem>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class CommandList;
	class ImageResource;

	/**
	 * @class FrameCapture
	 * @brief Writes the frame the renderer produces to a PNG (--capture-png, the parity harness): the display-referred
	 * frame_output as 8-bit sRGB, plus a sky mask (<name>_sky.png: white where nothing was drawn, depth 0 in reverse-Z)
	 * that tools/image_compare/xp_compare.py uses for the sky-only metrics.
	 *
	 * Request() / TakeResult() run on the main thread; Record() on the render thread, once per frame after the output
	 * is final and before overlays (text, reference images) are drawn on it.
	 */
	class FrameCapture
	{
	public:
		struct Result
		{
			bool success = false;
			std::filesystem::path png;
			std::filesystem::path skyMask;		// Empty when no mask was written
			std::string message;				// What went wrong, or "W x H"
		};

		/* @brief Captures the next frame the renderer finishes. A request made while one is pending replaces it. */
		static void Request(const std::filesystem::path &png, bool writeSkyMask = true);

		/* @brief A request is queued or its copy hasn't been read back yet. */
		static bool IsBusy();

		/* @brief Moves the last finished capture's result out; false while none is waiting. */
		static bool TakeResult(Result &out);

		/**
		 * @brief Render thread: writes the captures this command list copied last time it was recorded, then copies
		 * the output (and the depth for the sky mask) of a queued request into readback buffers.
		 * @param output frame_output, display-referred linear RGB (0-1 shows as 0-255 after sRGB encoding).
		 * @param depth The reverse-Z depth at render resolution, or nullptr for no sky mask.
		 */
		static void Record(CommandList *cmdList, ImageResource *output, ImageResource *depth);

		/**
		 * @brief Writes the raw texels of an image (mip 0, no header, rows top to bottom) to a file a few frames later:
		 * --dump-brdf-lut writes the GPU's BRDF table in X-Plane's brdf_lookup layout for BrdfLutTests.
		 */
		static void RequestDump(const std::filesystem::path &file);

		/* @brief Render thread: copies the image of a queued RequestDump() (any format with a known texel size). */
		static void RecordDump(CommandList *cmdList, ImageResource *image);

		/* @brief Releases the readback buffers. Call before the device goes away. */
		static void Shutdown();
	};
}

// -------------------------------------------------------
