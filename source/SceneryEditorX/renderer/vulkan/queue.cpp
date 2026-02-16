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
 * queue.cpp
 * -------------------------------------------------------
 * Created: 10/02/2026
 * -------------------------------------------------------
 */
#include "queue.h"
#include "renderer.h"
#include "swapchain.h"
#include <tracy/Tracy.hpp>

// -------------------------------------------------------

namespace SceneryEditorX
{

	VkSemaphore InitSemaphore(VkDevice device)
	{
	    VkSemaphoreCreateInfo CreateInfo = {
	        .sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO, 
	        .pNext = nullptr, 
	        .flags = 0
	    };
	
	    VkSemaphore semaphore;
	    VkResult result = vkCreateSemaphore(device, &CreateInfo, nullptr, &semaphore);
	    SEDX_VK_RESULT_ASSERT(result, "Semaphore creation failed");
	    return semaphore;
	}

    // -------------------------------------------------------

	std::array<Ref<Queue>, static_cast<uint32_t>(QueueType::Unknown)> Queue::regular = {};
	void *Queue::graphics = nullptr;
	void *Queue::compute = nullptr;
	void *Queue::copy = nullptr;

	uint32_t Queue::indexGraphics = (std::numeric_limits<uint32_t>::max)();
	uint32_t Queue::indexCompute = (std::numeric_limits<uint32_t>::max)();
	uint32_t Queue::indexCopy = (std::numeric_limits<uint32_t>::max)();
	
	std::map<ResourceType, std::vector<void *>> Queue::deletionQueue = {};
	std::mutex Queue::mutexDeletionQueue;
	
	// Helper: find a physical device queue family index suitable for the requested GPUQueueType.
	static uint32_t FindQueueFamily(VkPhysicalDevice physicalDevice, const QueueType type, VkSurfaceKHR surface)
	{
	    uint32_t queueFamilyCount = 0;
	    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);
	
	    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	    vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());
	
	    // Prefer specialized queues (compute-only / transfer-only) where applicable.
	    if (type == QueueType::Compute)
	    {
	        // Prefer a compute-only queue (no graphics bit).
	        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	        {
	            if ((queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) &&
	                (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
	            {
	                return i;
	            }
	        }
	        // Fallback: any compute-capable queue.
	        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	        {
	            if (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT)
	            {
	                return i;
	            }
	        }
	    }
	    else if (type == QueueType::Transfer)
	    {
	        // Prefer a transfer-only queue (no graphics and no compute).
	        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	        {
	            if ((queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT) &&
	                (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0 &&
	                (queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0)
	            {
	                return i;
	            }
	        }
	        // Fallback: any transfer-capable queue.
	        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	        {
	            if (queueFamilies[i].queueFlags & VK_QUEUE_TRANSFER_BIT)
	            {
	                return i;
	            }
	        }
	    }
	    else // GRAPHICS or default
	    {
	        // Prefer a queue with graphics (and, if surface is provided, present support).
	        if (surface != VK_NULL_HANDLE)
	        {
	            for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	            {
	                VkBool32 presentSupport = VK_FALSE;
	                vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);
	                if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && presentSupport)
	                    return i;
	            }
	        }
	
	        // Fallback: any graphics-capable queue.
	        for (uint32_t i = 0; i < queueFamilies.size(); ++i)
	        {
	            if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
	            {
	                return i;
	            }
	        }
	    }
	
	    return INVALID_VK_INDEX;
	}
	
    // -------------------------------------------------------

    Queue::Queue(const Ref<Device>& device, const QueueType type, const char *name) : name(name)
	{
		m_Device = device;  // Use the passed device directly
		m_Type = type;
	}


    Queue::~Queue()
    {
        m_Device.Reset();
    }

	// NOTE:
	// Several public methods in Queue are declared static in the header.
	// Static methods do not have access to instance members (m_...). To operate on actual Vulkan
	// queues we use the static `Queue::regular` array which holds Ref<Queue> objects created by Device.
	// The implementations below reference the appropriate Queue instance via Queue::GetQueue(...).


    /**
	 * @brief Wait for the specified queue to become idle.
	 * @param queue The Queue instance to wait on.
	 */
	void Queue::WaitIdle(const Queue &queue)
	{
        SEDX_CORE_ASSERT(queue.m_Device.IsValid(), "Invalid device in Queue::WaitIdle");
        SEDX_CORE_ASSERT(queue.m_Queue.handle != VK_NULL_HANDLE, "Invalid queue handle in Queue::WaitIdle");

		if (queue.m_Queue.handle != VK_NULL_HANDLE)
		{
		    vkQueueWaitIdle(queue.m_Queue.handle);
		}

	}

	/**
	 * @brief Initialize the queue by finding the appropriate queue family and retrieving the queue handle.
	 * Note: Swapchain is not accessed here to avoid initialization order issues - it's retrieved dynamically when needed.
	 */
	void Queue::Init()
	{
		VkPhysicalDevice phys = m_Device->GetPhysicalDevice();
		VkDevice logical = m_Device->GetLogicalDevice();
		VkSurfaceKHR surface = m_Device->GetWindowSurface();

		uint32_t familyIndex = FindQueueFamily(phys, m_Type, surface);
		SEDX_CORE_ASSERT(familyIndex != INVALID_VK_INDEX, "Failed to find suitable queue family for requested Queue type.");

		m_Queue.familyIndex = familyIndex;
		vkGetDeviceQueue(logical, m_Queue.familyIndex, 0, &m_Queue.handle); // Retrieve the device queue handle for the chosen family.

		CreateSemaphores();
	}

    /**
	 * @brief Destroy the queue and its associated resources, including semaphores. This should be called when the queue is no longer needed to free Vulkan resources.
	 */
	void Queue::Destroy()
	{
		SEDX_CORE_ASSERT(m_Device.IsValid(), "Invalid device in Queue::Destroy");
        SEDX_CORE_ASSERT(m_RenderSemaphore != VK_NULL_HANDLE, "Invalid render semaphore in Queue::Destroy");
        SEDX_CORE_ASSERT(m_PresentSemaphore != VK_NULL_HANDLE, "Invalid present semaphore in Queue::Destroy");

	    vkDestroySemaphore(m_Device->GetLogicalDevice(), m_RenderSemaphore, nullptr);
	    vkDestroySemaphore(m_Device->GetLogicalDevice(), m_PresentSemaphore, nullptr);

        m_RenderSemaphore = VK_NULL_HANDLE;
        m_PresentSemaphore = VK_NULL_HANDLE;
	}

	/**
	 * @brief Acquire the next image from the swapchain for rendering. This method uses the Graphics queue instance to call vkAcquireNextImageKHR and returns the index of the acquired image. The caller is responsible for ensuring that the Graphics queue is properly initialized and that the swapchain is valid before calling this method.
	 * @return The index of the acquired swapchain image.
	 */
	uint32_t Queue::AcquireNextImage()
	{
		// Acquire next image from the swapchain using the Graphics queue instance
		Queue *q = Queue::GetQueue(QueueType::Graphics);
		SEDX_CORE_ASSERT(q != nullptr, "No Graphics queue available for Queue::AcquireNextImage.");

		// Retrieve swapchain dynamically to avoid initialization order issues
		Swapchain* swapchain = Renderer::GetSwapChain();
		SEDX_CORE_ASSERT(swapchain != nullptr, "Swapchain not initialized");
		VkSwapchainKHR swapchainHandle = swapchain->Get();
		SEDX_CORE_ASSERT(swapchainHandle != VK_NULL_HANDLE, "Invalid swapchain handle");

		uint32_t ImageIndex = 0;
		VkResult result = vkAcquireNextImageKHR(q->m_Device->GetLogicalDevice(), swapchainHandle, UINT64_MAX, q->m_PresentSemaphore, nullptr, &ImageIndex);
		SEDX_CORE_ASSERT(result == VK_SUCCESS, "vkAcquireNextImageKHR failed");
		return ImageIndex;
	}
	
    // TODO: Replace the VkCommandBuffer parameter with a higher-level CommandList or CommandBuffer wrapper that manages command recording and submission more robustly. 
    // This would allow for better error handling, resource management, and integration with the rest of the rendering system. 
    // The current implementation assumes the caller is responsible for ensuring the command buffer is in a valid state for submission, 
    // which can lead to issues if not handled carefully.
	void Queue::SubmitSync(VkCommandBuffer cmdBuffer)
	{
	    // Submit a command buffer synchronously on the Graphics queue
	    Queue *q = Queue::GetQueue(QueueType::Graphics);
	    SEDX_CORE_ASSERT(q != nullptr, "No Graphics queue available for Queue::SubmitSync.");
	
	    VkSubmitInfo SubmitInfo = {
	        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	        .pNext = nullptr,
	        .waitSemaphoreCount = 0,
	        .pWaitSemaphores = VK_NULL_HANDLE,
	        .pWaitDstStageMask = VK_NULL_HANDLE,
	        .commandBufferCount = 1,
	        .pCommandBuffers = &cmdBuffer,
	        .signalSemaphoreCount = 0,
	        .pSignalSemaphores = VK_NULL_HANDLE,
	    };
	
	    VkResult result = vkQueueSubmit(q->m_Queue.handle, 1, &SubmitInfo, VK_NULL_HANDLE);
	    SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to submit queue.");
	}
	
	void Queue::SubmitAsync(VkCommandBuffer cmdBuffer)
	{
	    // Submit asynchronously on the Graphics queue using the present/render semaphores of that queue instance
	    Queue *q = Queue::GetQueue(QueueType::Graphics);
	    SEDX_CORE_ASSERT(q != nullptr, "No graphics queue available for SubmitAsync.");
        SEDX_CORE_ASSERT(q->m_PresentSemaphore != VK_NULL_HANDLE, "Invalid present semaphore in Queue::SubmitAsync.");
	    SEDX_CORE_ASSERT(q->m_RenderSemaphore != VK_NULL_HANDLE, "Invalid render semaphore in Queue::SubmitAsync.");
	
	    VkPipelineStageFlags waitFlags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	
	    VkSubmitInfo SubmitInfo = {
	        .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
	        .pNext = nullptr,
	        .waitSemaphoreCount = 1,
	        .pWaitSemaphores = &q->m_PresentSemaphore,
	        .pWaitDstStageMask = &waitFlags,
	        .commandBufferCount = 1,
	        .pCommandBuffers = &cmdBuffer,
	        .signalSemaphoreCount = 1,
	        .pSignalSemaphores = &q->m_RenderSemaphore,
	    };
	
	    VkResult result = vkQueueSubmit(q->m_Queue.handle, 1, &SubmitInfo, VK_NULL_HANDLE);
	    SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to submit queue.");
	}

	/**
	 * @brief Present the rendered image to the screen by submitting a present request to the graphics queue. This method uses the render semaphore to ensure that rendering is complete before presentation and waits for the presentation to finish across all queues to maintain synchronization. The caller must ensure that the image index provided is valid and corresponds to an acquired swapchain image.
	 * @param imageIdx The index of the swapchain image to present, which should have been acquired using AcquireNextImage() and rendered to before calling this method.
	 */
	void Queue::Present(uint32_t imageIdx)
	{
		Queue *q = GetQueue(QueueType::Graphics);
		SEDX_CORE_ASSERT(q != nullptr, "No Graphics queue available for Queue::Present.");
		SEDX_CORE_ASSERT(q->m_RenderSemaphore != VK_NULL_HANDLE, "Invalid render semaphore in Queue::Present.");

		// Retrieve swapchain dynamically to avoid initialization order issues
		Swapchain* swapchain = Renderer::GetSwapChain();
		SEDX_CORE_ASSERT(swapchain != nullptr, "Swapchain not initialized");
		VkSwapchainKHR swapchainHandle = swapchain->Get();
		SEDX_CORE_ASSERT(swapchainHandle != VK_NULL_HANDLE, "Invalid swapchain handle in Queue::Present.");

		VkPresentInfoKHR PresentInfo = {
			.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
			.pNext = nullptr,
			.waitSemaphoreCount = 1,
			.pWaitSemaphores = &q->m_RenderSemaphore,
			.swapchainCount = 1,
			.pSwapchains = &swapchainHandle,
			.pImageIndices = &imageIdx,
			//.pResults = NULL,
		};

		VkResult result = vkQueuePresentKHR(q->m_Queue.handle, &PresentInfo);
		SEDX_CORE_ASSERT(result == VK_SUCCESS, "Failed to present queue.");

		// Wait for presentation to finish across all queues (keeps previous behavior)
		WaitIdle(*q);
	}
	
	// -------------------------------------------------------
	
	uint32_t Queue::GetQueueIndex(const QueueType type)
	{
	    if (type == QueueType::Graphics)
	        return indexGraphics;
	
	    if (type == QueueType::Transfer)
	        return indexCopy;
	
	    if (type == QueueType::Compute)
	        return indexCompute;
	
	    return 0;
	}
	
	Queue *Queue::GetQueue(const QueueType type)
	{
	    if (type == QueueType::Graphics)
	        return regular[static_cast<uint32_t>(QueueType::Graphics)].Get();
	
	    if (type == QueueType::Compute)
	        return regular[static_cast<uint32_t>(QueueType::Compute)].Get();
	
	    if (type == QueueType::Transfer)
	        return regular[static_cast<uint32_t>(QueueType::Transfer)].Get();
	
	    return nullptr;
	}
	
	void *Queue::GetQueueResource(const QueueType type)
	{
	    if (type == QueueType::Graphics)
	        return graphics;
	
	    if (type == QueueType::Transfer)
	        return copy;
	
	    if (type == QueueType::Compute)
	        return compute;
	
	    return nullptr;
	}
	
	void Queue::QueueWaitAll(const bool /*flush*/)
	{
	    // Wait on all registered queues
	    for (uint32_t i = 0; i < static_cast<uint32_t>(QueueType::Count); ++i)
	    {
	        if (Queue::regular[i])
	        {
                Queue::regular[i]->WaitIdle(*Queue::regular[i]);
	        }
	    }
	}
	
	// -------------------------------------------------------
	
	void Queue::AddDeletionQueue(const ResourceType resourceType, void *resource)
	{
	    if (!resource)
	        return;
	
	    std::scoped_lock guard(Queue::mutexDeletionQueue);
	    Queue::deletionQueue[resourceType].emplace_back(resource);
	}
	
	void Queue::ParseDeletionQueue()
	{
	    std::scoped_lock guard(mutexDeletionQueue);
	
		Ref<Device> device;
	    for (auto &it : deletionQueue)
	    {
	        ResourceType resourceType = it.first;
	        for (auto resource : it.second)
	        {
	            switch (resourceType)
	            {
	            case ResourceType::Image: /*MemoryAllocator::DestroyMemoryTexture(resource);*/
	                break;
	            case ResourceType::ImageView:
                    vkDestroyImageView(device->GetLogicalDevice(), static_cast<VkImageView>(resource), nullptr);
	                break;
	            case ResourceType::Sampler:
                    vkDestroySampler(device->GetLogicalDevice(), reinterpret_cast<VkSampler>(resource), nullptr);
	                break;
	            case ResourceType::Buffer: /*MemoryAllocator::DestroyMemoryBuffer(resource); */
	                break;
	            case ResourceType::Shader:
                    vkDestroyShaderModule(device->GetLogicalDevice(), static_cast<VkShaderModule>(resource), nullptr);
	                break;
	            case ResourceType::Semaphore:
                    vkDestroySemaphore(device->GetLogicalDevice(), static_cast<VkSemaphore>(resource), nullptr);
	                break;
	            case ResourceType::Fence:
                    vkDestroyFence(device->GetLogicalDevice(), static_cast<VkFence>(resource), nullptr);
	                break;
	            case ResourceType::DescriptorSetLayout:
                    vkDestroyDescriptorSetLayout(device->GetLogicalDevice(), static_cast<VkDescriptorSetLayout>(resource), nullptr);
	                break;
	            case ResourceType::QueryPool:
	                vkDestroyQueryPool(device->GetLogicalDevice(), static_cast<VkQueryPool>(resource), nullptr);
	                break;
	            case ResourceType::Pipeline:
                    vkDestroyPipeline(device->GetLogicalDevice(), static_cast<VkPipeline>(resource), nullptr);
	                break;
	            case ResourceType::PipelineLayout:
                    vkDestroyPipelineLayout(device->GetLogicalDevice(), static_cast<VkPipelineLayout>(resource), nullptr);
	                break;
	            case ResourceType::AccelerationStructure:
                    vkDestroyAccelerationStructureKHR(device->GetLogicalDevice(), static_cast<VkAccelerationStructureKHR>(resource), nullptr);
	                break;
	            default:
	                SEDX_CORE_ASSERT(false, "Unknown resource");
	                break;
	            }
	
	            /*// Delete descriptor sets which are now invalid (because they are referring to a deleted resource)
		                if (resourceType == ResourceType::ImageView || resourceType == ResourceType::Buffer)
		                {
		                    for (auto it = Descriptor::sets.begin(); it != Descriptor::sets.end();)
		                    {
		                        if (it->second.IsReferingToResource(resource)) { it = Descriptor::sets.erase(it); }
		                        else { ++it; }
		                    }
		                }*/
	        }
	    }

	    deletionQueue.clear();
	}
	
	bool Queue::ParseDeletionQueueNeedsTo()
	{
	    static uint32_t framesEquilibrium = 0;
	    static uint32_t objectsToDeletePrevious = 0;
	
	    // Count deletions in the queue
	    uint32_t objectsToDelete = 0;
	    for (uint32_t i = 0; i < static_cast<uint32_t>(ResourceType::MaxEnum); i++)
	    {
	        objectsToDelete += static_cast<uint32_t>(Queue::deletionQueue[static_cast<ResourceType>(i)].size());
	    }
	
	    // Check if the number of objects to delete has remained unchanged
	    if (objectsToDelete > 0 && objectsToDelete == objectsToDeletePrevious)
	    {
	        framesEquilibrium++;
	
	        /*// If it’s been stable for frame_self life frames, reset counter and delete
		            if (framesEquilibrium >= renderer_resource_frame_lifetime)
		            {
		                framesEquilibrium = 0;
		                return true;
		            }*/
	    }
	    else
	    {
	        // Reset counter if the count changed or if nothing is in the queue
	        framesEquilibrium = 0;
	    }
	
	    // Tick the previous object count to the current count
	    objectsToDeletePrevious = objectsToDelete;
	
	    return false;
	}
	
	void Queue::CreateSemaphores()
	{
        m_PresentSemaphore = InitSemaphore(m_Device->GetLogicalDevice());
        m_RenderSemaphore = InitSemaphore(m_Device->GetLogicalDevice());
	}
	
}

// -------------------------------------------------------
