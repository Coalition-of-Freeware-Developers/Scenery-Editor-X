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
 * queue_manager.h
 * -------------------------------------------------------
 * Created: 10/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "command_list.h"
#include "command_pool.h"
#include "queue.h"
#include <limits>
#include <vma/vk_mem_alloc.h>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class Device;
	class Queue;
	
	/**
	 * @class QueueManager
	 * @brief Manages GPU queues and their associated command lists.
	 *
	 * This class is responsible for allocating and freeing GPU queues
	 * and managing their command lists. It supports configuration options
	 * such as the number of command lists per queue and serialization
	 * by queue type. 
	 *
	 * - Creating and destroying physical device queues (VkQueue handles).
	 * - Managing per-queue command pools and synchronization primitives.
	 * - Allocating and returning Queue objects (logical wrappers) to the engine or renderer.
	 *
	 * If you need to know more about Vulkan Queues look at the Vulkan documentation:
	 * @see https://docs.vulkan.org/guide/latest/queues.html
	 *
	 * @code
	 * Ref<QueueManager> queueManager = device->GetQueueManager();
	 * queueManager->AllocateQueue(QueueType::Graphics, 2, "Main Graphics Queue");
	 * Ref<Queue> graphicsQueue = queueManager->GetQueue(QueueType::Graphics);
	 * graphicsQueue->Submit(...);
	 * queueManager->FreeQueue(graphicsQueue);
	 * @endcode
	 *
	 * Note: QueueManager does not currently support multiple queues of the same type, but this could be added in the future if needed. 
	 * The current design assumes one queue per type (graphics, compute, transfer, present) for simplicity.
	 */
	class QueueManager : public RefCounted
	{
	public:
		/**
		 * @struct QueueConfig
		 * @brief Configuration options for the QueueManager.
		 * @param cmdListsPerQueue Number of reusable command lists per queue.
		 * @param serializeByType Whether to serialize access to queues by type.
		 */
		struct QueueConfig
		{
			uint32_t cmdListsPerQueue = 2; // Number of reusable command lists per queue
			bool serializeByType = false;  // Whether to serialize access to queues by type
		};

		/**
		 * @struct QueueFamilyIndices
		 * @brief Structure to hold the indices of different queue families.
		 * Each member is initialized to the maximum uint32_t value to indicate an invalid index by default.
		 */
		struct QueueFamilyIndices
		{
			uint32_t graphics = (std::numeric_limits<uint32_t>::max)();
			uint32_t compute  = (std::numeric_limits<uint32_t>::max)();
			uint32_t transfer = (std::numeric_limits<uint32_t>::max)();
			uint32_t present  = (std::numeric_limits<uint32_t>::max)();
		};
	
		/**
		 * @brief Detect the queue families supported by the given physical device.
		 * @param physicalDevice Reference to the physical device to query.
		 * @return QueueFamilyIndices structure containing the detected queue family indices.
		 */
		static QueueFamilyIndices DetectQueueFamilies(const VkPhysicalDevice &physicalDevice);
	
		/**
		 * @brief Build the VkDeviceQueueCreateInfo structures based on the detected queue families.
		 * @param indices Detected queue family indices.
		 * @param queueInfo Vector to store the created VkDeviceQueueCreateInfo structures.
		 * @param priority Vector to store the queue priorities.
		 */
		static void BuildQueueInfo(const QueueFamilyIndices &indices, std::vector<VkDeviceQueueCreateInfo> &queueInfo, std::vector<float> &priority);
	
		/**
		 * @brief Construct a new Queue Manager object with the given configuration.
		 * @param device Reference to the Device object.
		 * @param config Configuration options for the QueueManager.
		 */
		QueueManager(const Ref<Device> &device, const QueueConfig &config = {});
		virtual ~QueueManager() override;
	
		QueueManager(const QueueManager &) = delete;
		QueueManager &operator=(const QueueManager &) = delete;
	
		/**
		 * @brief Allocate a GPU queue of the specified type.
		 * @param type The type of queue to allocate.
		 * @param preAllocCmdList Number of pre-allocated command lists for this queue.
		 * @param name Optional name for the queue.
		 * @return Reference to the allocated Queue.
		 */
		void AllocateQueue(QueueType type, uint32_t preAllocCmdList = 0, const char *name = nullptr);
	
		/**
		 * @brief Free a previously allocated GPU queue.
		 * @param queue Reference to the Queue to free.
		 */
		void FreeQueue(Ref<Queue> queue);
	
		/**
		 * @brief Iterate all physical queues and call vkQueueWaitIdle under their mutex.
		 * This function blocks until all queues have completed their operations.
		 */
		static void WaitIdleAll(const bool flush = false);
	
		/**
		 * @brief Return a pointer to the Queue for the given QueueType.
		 * @param type The QueueType to allocate.
		 * @return Pointer to Queue inside m_GPUQueues or nullptr if out-of-range.
		 */
		Ref<Queue> *GetQueue(QueueType type);
	
		/**
		 * @brief Get the family index of a GPU queue.
		 * @return The family index of a GPU queue.
		 */
		static uint32_t GetFamilyIndex(const Ref<Queue> &queue);
	
		/**
		 * @brief Get the Vulkan queue handle of a GPU queue.
		 * @param queue The GPUQueue to query.
		 * @return The VkQueue handle of the GPU queue.
		 */
		static VkQueue GetQueueHandle(const Ref<Queue> &queue);
	
		/**
		 * @brief Accessor that returns all GPU queues.
		 * @return A vector containing references to all GPU queues.
		 */
		std::vector<Ref<Queue>> GetAllQueues() const { return m_GPUQueues; }
	
		/**
		 * @brief Const overload that returns a pointer to the GPUQueue for the given QueueType.
		 * @param type The QueueType to query.
		 * @return Pointer to GPUQueue inside m_GPUQueues or nullptr if out-of-range.
		 */
		const Ref<Queue> *GetQueue(QueueType type) const;
	
		/**
		 * @brief Convenience accessor that returns the Vulkan queue handle for a QueueType.
		 * @param type The QueueType to query.
		 * @return VkQueue handle or VK_NULL_HANDLE if the queue is not available.
		 */
		VkQueue GetQueueHandleByType(QueueType type) const;
	
		/**
		 * @brief Convenience accessor that returns the family index for a QueueType.
		 * @param type The QueueType to query.
		 * @return Queue family index or Unknown if the queue is not available.
		 */
		uint32_t GetFamilyIndexByType(QueueType type) const;
	
		/**
		 * @brief Add a resource to the deletion queue for deferred destruction.
		 * @param type The type of resource to delete.
		 * @param resource Pointer to the resource to delete.
		*/
		static void AddDeletionQueue(ResourceType type, void *resource);

		/**
		 * @brief Add a resource and optional allocation metadata to the deletion queue.
		 * @param type The type of resource to delete.
		 * @param resource Pointer to the resource to delete.
		 * @param allocation The VMA allocation associated with the resource.
		 */
		static void AddDeletionQueue(ResourceType type, void *resource, VmaAllocation allocation);
	
		/**
		 * @brief Parse and process the deletion queue, destroying resources as needed.
		 */
		static void ParseDeletionQueue();

		/**
		 * @brief Check if the deletion queue needs to be parsed.
		 * @return True if the deletion queue needs to be parsed, false otherwise.
		 */
		static bool NeedToParseDeletionQueue();

		/**
		 * @brief Signal that the renderer has fully shut down and no further
		 * resources will legitimately be enqueued for deletion.
		 *
		 * After this call any AddDeletionQueue invocation (e.g. from Ref<> destructors
		 * running in the CRT static-dtor phase) is silently discarded, preventing a
		 * use-after-free on the already-destroyed s_DeletionQueue map.
		 * Must be called at the very end of Renderer::Shutdown(), after the final
		 * ParseDeletionQueue() flush.
		 */
		static void NotifyShutdown();

		/**
		 * @brief Get the next available command list for the current queue index, cycling through the pre-allocated command lists.
		 * @return Reference to the next CommandList for the current queue index.
		 */
		CommandList *NextCommandList();

	private:
		Ref<Device> m_Device = nullptr;
		QueueConfig m_Config;
		CommandPool m_CmdPool;  // Declared before m_CmdLists so the pool outlives the command lists during destruction.
		std::array<Ref<CommandList>, 2> m_CmdLists = { nullptr };
		QueueFamilyIndices m_FamilyIndices;
		std::atomic<uint32_t> m_Index = 0;
		std::vector<Ref<Queue>> m_GPUQueues; // Available GPU queues indexed by QueueType
	};
	
}

// -------------------------------------------------------
