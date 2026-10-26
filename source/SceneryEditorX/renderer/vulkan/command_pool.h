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
 * command_pool.h
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include <SceneryEditorX/utils/pointers.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Device;

	/**
	 * @enum CommandPoolType
	 * @brief Defines the type of command pool, which determines the behavior and usage of command buffers allocated from the pool.
	 */
	enum class CommandPoolType
	{
		Transient,  // Command buffers allocated from this pool will be short-lived and reset or freed in a relatively short timeframe.
		Resettable, // Command buffers allocated from this pool can be individually reset to the initial state.
		Protected,  // Command buffers allocated from this pool can be submitted to protected queues and may contain protected commands.
		MaxEnum
	};

	/**
	 * @class CommandPool
	 * @brief Encapsulates a Vulkan command pool, providing functionality to create and manage command buffers for recording GPU commands.
	 * The CommandPool class is responsible for allocating command buffers from the Vulkan device and ensuring proper cleanup of resources when the pool is destroyed.
	 */
	class CommandPool 
	{
	public:
		CommandPool() = default;

		/**
		 * @brief Creates a command pool by fetching the device from RenderContext (only valid after RenderContext is fully initialized).
		 * @param queueFamilyIndex The index of the queue family for which the command pool is created.
		 * @param pool The type of command pool to create.
		 */
		CommandPool(uint32_t queueFamilyIndex, CommandPoolType pool = CommandPoolType::MaxEnum);

		/**
		 * @brief Creates a command pool using an explicitly supplied device (safe to use during Device construction).
		 * @param device The device to use for creating the command pool.
		 * @param queueFamilyIndex The index of the queue family for which the command pool is created.
		 * @param pool The type of command pool to create.
		 */
		CommandPool(const Ref<Device>& device, uint32_t queueFamilyIndex, CommandPoolType pool = CommandPoolType::MaxEnum);

		/**
		 * @brief Destroys the command pool and releases all associated resources.
		 */
		~CommandPool();

		// Non-copyable
		CommandPool(const CommandPool&) = delete;
		CommandPool& operator=(const CommandPool&) = delete;
	
		// Movable
		CommandPool(CommandPool&& other) noexcept;
		CommandPool& operator=(CommandPool&& other) noexcept;

		/**
		 * @brief Allocate `count` primary command buffers from the owned pool.
		 * @param count The number of command buffers to allocate.
		 * @return A vector containing the allocated command buffers.
		 */
		std::vector<VkCommandBuffer> Allocate(uint32_t count) const;

		/**
		 * @brief Get the Vulkan command pool handle.
		 * @return The Vulkan command pool handle.
		 */
		[[nodiscard]] VkCommandPool GetPool() const { return m_CmdPool; }

		/**
		 * @brief Get the device this command pool was created with.
		 * @return Reference to the owning Device. Safe to call during Device construction.
		 */
		[[nodiscard]] Ref<Device> GetDevice() const;
	
		/**
		 * @brief Explicitly destroy the command pool before m_Device destruction.
		 */
		void Destroy();
	
	private:
		Ref<Device> m_Device; // Store m_Device so we can destroy the pool in the destructor (RAII)
		VkCommandPool m_CmdPool = VK_NULL_HANDLE ;
		CommandPoolType m_PoolType = CommandPoolType::MaxEnum;
	};

}

// -------------------------------------------------------
