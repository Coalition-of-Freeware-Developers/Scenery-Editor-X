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
 * render_data.h
 * -------------------------------------------------------
 * Created: 11/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include <array>
#include <limits>
#include <string>
#include <vma/vk_mem_alloc.h>

// -----------------------------------------------------------------

namespace SceneryEditorX
{

	/* Constant representing an invalid Vulkan index. */
	#define INVALID_VK_INDEX 0xFFFFFFFF

	/* Vulkan API version supported by the m_Device */
	struct ApiVersion
	{
		uint32_t variant = 0, major = 0, minor = 0, patch = 0; // Vulkan version is encoded as: variant(2 bits), major(10 bits), minor(10 bits), patch(10 bits)

		/**
		 * @brief Returns the Vulkan version as a string.
		 * @return A string representing the Vulkan version in the format "major.minor.patch".
		 */
		[[nodiscard]] std::string VulkanVersionStr() const
		{
			return std::to_string(major) + "." + std::to_string(minor) + "." + std::to_string(patch);
		}
	};

#pragma region Renderer Limits

	constexpr uint8_t  MAX_CONSTANT_BUFFER_COUNT		= 8;
	constexpr uint32_t MAX_ARRAY_SIZE					= 16384;

	constexpr uint32_t MIP_LIST							= std::numeric_limits<uint32_t>::max();
	constexpr uint32_t ALL_MIPS							= std::numeric_limits<uint32_t>::max();

	static constexpr uint32_t MAX_FRAMES_IN_FLIGHT		= 2;
	static constexpr uint32_t MAX_BUFFER_UPDATE_SIZE	= 65536; // vkCmdUpdateBuffer has a limit of 65536 bytes
	static constexpr uint32_t MAX_DRAW_CALLS			= 20000; // Maximum number of draw calls per frame; used to size CPU-side staging arrays for draw data, etc.
	static constexpr uint32_t MAX_INSTANCE_COUNT		= 1024;  // Maximum number of instances per draw call; used to size instance data buffers and limit draw calls to avoid GPU timeouts
	static constexpr uint32_t MAX_RESOURCE_LIFETIME		= 100;	 // Maximum number of frames a resource can live; used to manage resource lifetimes and avoid GPU timeouts
	static constexpr uint32_t DRAW_DATA_BUFFER_COUNT	= 41;	 // Matches the command list pool size to avoid cpu-gpu memcpy races

#pragma endregion

#pragma region Shader Data

	struct ShaderData
	{
		std::array<float, 16> viewProjection{};
	};

	struct ShaderDataBuffer
	{
		VkBuffer buffer = VK_NULL_HANDLE;
		VmaAllocation allocation = VK_NULL_HANDLE;
		void *mapped = nullptr;
		VkDeviceAddress deviceAddress = 0;
	};

#pragma endregion

}

// -----------------------------------------------------------------
