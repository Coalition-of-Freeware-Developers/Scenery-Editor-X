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
 * vulkan_support.h
 * -------------------------------------------------------
 * Created: 20/10/2026
 * -------------------------------------------------------
 */
#pragma once
#include <cstdint>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	/* @brief Whether this machine can run the renderer. The values are also the exit codes of --check-vulkan (the installer reads them). */
	enum class VulkanSupport : uint8_t
	{
		Supported = 0,		// A GPU with Vulkan 1.3
		NoLoader = 2,		// vulkan-1.dll, the Vulkan runtime, is missing
		LoaderTooOld = 3,	// The Vulkan runtime is older than 1.3
		NoDriver = 4,		// No graphics driver offers Vulkan (no instance, or no GPU)
		DriverTooOld = 5	// GPUs found, none with Vulkan 1.3 (an old driver or an old GPU)
	};

	struct VulkanSupportReport
	{
		VulkanSupport status = VulkanSupport::NoLoader;
		uint32_t runtimeVersion = 0;	// The Vulkan runtime's version (VK_MAKE_API_VERSION), 0 without one
		std::string bestDevice;			// The GPU with the newest Vulkan and that version ("NVIDIA GeForce RTX 4090, Vulkan 1.4.312")
	};

	/**
	 * @brief Checks for what the renderer needs (Vulkan 1.3): the runtime, an instance and a GPU. Creates and destroys a bare
	 * instance; RenderContext::Init loads its own function pointers afterwards.
	 */
	VulkanSupportReport CheckVulkanSupport();

	/* @brief What a report means for the user and what to do about it (empty when Vulkan is supported). */
	std::string DescribeVulkanSupport(const VulkanSupportReport &report);
}

// -------------------------------------------------------
