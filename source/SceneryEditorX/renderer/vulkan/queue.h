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
 * queue.h
 * -------------------------------------------------------
 * Created: 10/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "enums.h"
#include "render_data.h" 
#include "SceneryEditorX/core/resource/iobject.h"
#include "SceneryEditorX/utils/inheritance.h"
#include <mutex>

// -------------------------------------------------------

namespace SceneryEditorX
{
	class FrameSync;
	class Swapchain;
	class Device;
	class QueueManager;

	/**
	 * @struct GPUQueue
	 * @brief Encapsulates a Vulkan queue handle and its associated family index. This struct is used internally by the Queue class to manage Vulkan queues.
	 */
	struct GPUQueue
	{
		VkQueue handle = VK_NULL_HANDLE;         // Vulkan queue handle
		uint32_t familyIndex = INVALID_VK_INDEX; // Queue family index
	};

	/**
	 * @class Queue
	 * @brief Represents a GPU queue for command submission. 
	 * This class manages a Vulkan queue handle, its associated family index, 
	 * and synchronization primitives (semaphores) for rendering and presentation. 
	 * It provides methods for submitting command buffers and synchronizing queue operations. 
	 * Queue instances are created and managed by the QueueManager class.
	 */
	class Queue : public SharedObject
	{ 
	public:
		Queue(const Ref<Device>& device, const QueueType type, const char *name);
		virtual ~Queue() override;
	
		void Init();
		void Destroy();
		VkQueue GetQueue() const { return m_Queue.handle; }
		QueueType GetType() const { return m_Type; }

		/**
		 * @brief Submit pre-built synchronization work to this queue's underlying VkQueue using VkSubmitInfo2.
		 * @param submitInfo Fully populated VkSubmitInfo2 describing wait/signal semaphores and command buffers.
		 * @param fence Optional fence to signal when the batch is complete (VK_NULL_HANDLE for fire-and-forget).
		 */
		void Submit(const VkSubmitInfo2& submitInfo, VkFence fence = VK_NULL_HANDLE);

		/**
		 * @brief 
		 * @return 
		 */
		static uint32_t AcquireNextImage();

		/**
		 * @brief 
		 * @param cmdBuffer 
		 */
		static void SubmitSync(VkCommandBuffer cmdBuffer);

		/**
		 * @brief 
		 * @param cmdBuffer 
		 */
		static void SubmitAsync(VkCommandBuffer cmdBuffer);

		/**
		 * @brief 
		 * @param imageIdx 
		 */
		static void Present(uint32_t imageIdx);

		/**
		 * @brief 
		 * @param swapchain 
		 * @param imageIdx 
		 * @param waitSemaphore 
		 * @return 
		 */
		bool Present(Swapchain *swapchain, uint32_t imageIdx, FrameSync *waitSemaphore);

		static void WaitIdle(const Queue& queue);
		static void QueueWaitAll(const bool flush);
	
		static uint32_t GetQueueIndex(const QueueType type);
		static Queue *GetQueue(const QueueType type);
		uint32_t GetFamilyIndex() const { return m_Queue.familyIndex; }
		static void *GetQueueResource(const QueueType type);

		static void AddDeletionQueue(ResourceType resourceType, void *resource);
		static void ParseDeletionQueue();
		static bool ParseDeletionQueueNeedsTo();
	
		static std::array<Ref<Queue>, static_cast<uint32_t>(QueueType::Unknown)> regular;
		static void *graphics;
		static void *compute;
		static void *copy;
	
		static uint32_t indexGraphics;
		static uint32_t indexCompute;
		static uint32_t indexCopy;
	
		static std::map<ResourceType, std::vector<void *>> deletionQueue;
		static std::mutex mutexDeletionQueue;
	
	private:
		void CreateSemaphores();
		GPUQueue m_Queue;
		QueueType m_Type;
	
		Ref<Device> m_Device = nullptr;
		VkSwapchainKHR m_SwapChain = VK_NULL_HANDLE;
		VkSemaphore m_RenderSemaphore = VK_NULL_HANDLE;
		VkSemaphore m_PresentSemaphore = VK_NULL_HANDLE;
		const char *name;

		// Protects per-queue state (semaphores, swapchain handle, etc.) for thread-safe Present/Submit
		mutable std::mutex m_Mutex;
	};

}

// -------------------------------------------------------
