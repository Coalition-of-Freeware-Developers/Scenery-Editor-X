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
 * queue_manager.cpp
 * -------------------------------------------------------
 * Created: 10/02/2026
 * -------------------------------------------------------
 */
#include "queue_manager.h"
#include "device.h"
#include "render_context.h"
#include <algorithm>
#include <string>

// -------------------------------------------------------

namespace SceneryEditorX
{
	struct DeletionQueueEntry
	{
		void *resource = nullptr;
		VmaAllocation allocation = VK_NULL_HANDLE;
	};
	
#pragma region Static Members

	static constexpr uint32_t kQueueTypeSlots = static_cast<uint32_t>(QueueType::Present) + 1;
	static std::array<Ref<Queue>, kQueueTypeSlots> s_Regular; // indexed by QueueType value
	static std::mutex s_MutexAllocation;    // Mutex for thread-safe resource allocation
	static std::mutex s_MutexDeletionQueue; // Mutex for thread-safe deletion queue access
	static std::unordered_map<ResourceType, std::vector<DeletionQueueEntry>> s_DeletionQueue;
	
	// -------------------------------------------------------
	
	/**
	 * @brief Get the queue family index that matches the requested flags.
	 * @param flags The queue flags to match.
	 * @param familyProp The properties of the queue families.
	 * @param index The output index of the matching queue family.
	 * @return True if a matching queue family was found, false otherwise.
	 */
	static bool GetQueueFamilyIndex(const VkQueueFlagBits flags, const std::vector<VkQueueFamilyProperties> &familyProp, uint32_t *index)
	{
	
		// Try to find a queue that only supports compute (dedicated)
		if (flags & VK_QUEUE_COMPUTE_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(familyProp.size()); i++)
			{
				if ((familyProp[i].queueFlags & flags) && ((familyProp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0))
				{
					*index = i;
					return true;
				}
			}
		}
	
		// Try to find a queue that only supports copy (dedicated)
		if (flags & VK_QUEUE_TRANSFER_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(familyProp.size()); i++)
			{
				if ((familyProp[i].queueFlags & flags) && ((familyProp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
					((familyProp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					*index = i;
					return true;
				}
			}
		}
	
		// Try to find a queue that supports Sparse Binding operations (dedicated)
		if (flags & VK_QUEUE_SPARSE_BINDING_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(familyProp.size()); i++)
			{
				if ((familyProp[i].queueFlags & flags) && ((familyProp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
					((familyProp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					*index = i;
					return true;
				}
			}
		}
	
		// Try to find a queue that supports protected bit
		if (flags & VK_QUEUE_PROTECTED_BIT)
		{
			for (uint32_t i = 0; i < static_cast<uint32_t>(familyProp.size()); i++)
			{
				if ((familyProp[i].queueFlags & flags) && ((familyProp[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) &&
					((familyProp[i].queueFlags & VK_QUEUE_COMPUTE_BIT) == 0))
				{
					*index = i;
					return true;
				}
			}
		}
	
		// For graphics, just find any queue that supports graphics
		for (uint32_t i = 0; i < static_cast<uint32_t>(familyProp.size()); i++)
		{
			if (familyProp[i].queueFlags & flags)
			{
				*index = i;
				return true;
			}
		}
	
		return false;
	};
	
	/**
	 * @brief Convert QueueType enum to human-readable string representation
	 * @param type The QueueType to convert
	 * @return String representation of the queue type
	 */
	static constexpr const char *QueueToString(const QueueType type)
	{
		switch (type)
		{
		case QueueType::Graphics: return "Graphics";
		case QueueType::Compute:  return "Compute";
		case QueueType::Transfer: return "Transfer";
		case QueueType::Present:  return "Present";
		case QueueType::Unknown:  return "Unknown";
		default:
			return "Invalid";
		}
	}

#pragma endregion

	// ---------------------------------------------------------
	
	QueueManager::QueueFamilyIndices QueueManager::DetectQueueFamilies(const VkPhysicalDevice &physicalDevice)
	{
		QueueFamilyIndices indices;
		if (!physicalDevice)
		{
			return indices;
		}

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

		std::vector<VkQueueFamilyProperties> queueFamiliesProperties(queueFamilyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamiliesProperties.data());

		uint32_t index = 0;
		if (GetQueueFamilyIndex(VK_QUEUE_GRAPHICS_BIT, queueFamiliesProperties, &index))
		{
			indices.graphics = index;
		}
		else
		{
			SEDX_CORE_WARN_TAG("QueueManager", "Graphics queue not supported.");
		}
	
		if (GetQueueFamilyIndex(VK_QUEUE_COMPUTE_BIT, queueFamiliesProperties, &index))
		{
			indices.compute = index;
		}
		else
		{
			SEDX_CORE_WARN_TAG("QueueManager", "Compute queue not supported.");
		}
	
		if (GetQueueFamilyIndex(VK_QUEUE_TRANSFER_BIT, queueFamiliesProperties, &index))
		{
			indices.transfer = index;
		}
		else
		{
			SEDX_CORE_WARN_TAG("QueueManager", "Transfer queue not supported.");
		}
	
		// Present currently defaults to graphics until swapchain integration is finalized.
		indices.present = indices.graphics;
	
		return indices;
	}
	
	void QueueManager::BuildQueueInfo(const QueueFamilyIndices &indices, std::vector<VkDeviceQueueCreateInfo> &queueInfo, std::vector<float> &priority)
	{
		queueInfo.clear();
		priority.clear();
	
		const auto invalidIndex = (std::numeric_limits<uint32_t>::max)();
		const std::array<uint32_t, 3> requestedFamilies = {indices.graphics, indices.compute, indices.transfer};
		SEDX_CORE_TRACE_TAG("QueueManager", "Building Queue Create Infos for families \n"
							" - Graphics: {} \n"
							" - Compute: {} \n"
							" - Transfer: {}",
							indices.graphics, indices.compute, indices.transfer);
	
		std::array<uint32_t, 3> uniqueFamilies = {};
		uint32_t uniqueFamilyCount = 0;

		auto has_family = [&](uint32_t family) {
			return std::ranges::any_of(uniqueFamilies.begin(), uniqueFamilies.begin() + uniqueFamilyCount, [family](uint32_t existingFamily) {
				return existingFamily == family;
			});
		};

		for (uint32_t family : requestedFamilies)
		{
			if (family == invalidIndex || has_family(family))
			{
				continue;
			}

			uniqueFamilies[uniqueFamilyCount++] = family;
		}

		queueInfo.resize(uniqueFamilyCount);
		priority.resize(uniqueFamilyCount, 1.0f);

		for (uint32_t i = 0; i < uniqueFamilyCount; ++i)
		{
			const uint32_t family = uniqueFamilies[i];

			VkDeviceQueueCreateInfo createInfo{};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			createInfo.queueFamilyIndex = family;
			createInfo.queueCount = 1;
			createInfo.pQueuePriorities = &priority[i];
			queueInfo[i] = createInfo;

			SEDX_CORE_TRACE_TAG("QueueManager", "Added QueueCreateInfo for family index {}", family);
		}
	}
	
	QueueManager::QueueManager(const Ref<Device> &device, const QueueConfig &config) : m_Device(device), m_Config(config)
	{
		SEDX_CORE_TRACE_TAG("QueueManager", "=== Initializing Queue Manager ===");
	
		SEDX_CORE_ASSERT(m_Device, "Device must be initialized before QueueManager");
	
		m_FamilyIndices = DetectQueueFamilies(m_Device->GetPhysicalDevice());
		SEDX_CORE_TRACE_TAG("QueueManager", "Detected Queue Families - Graphics: {}, Compute: {}, Transfer: {}, Present: {}",
						   m_FamilyIndices.graphics, m_FamilyIndices.compute, m_FamilyIndices.transfer, m_FamilyIndices.present);
	
	   // Reserve slots indexed by QueueType numeric value.
		m_GPUQueues.resize(kQueueTypeSlots);

		// Initialize only valid allocatable queue types.
		constexpr std::array<QueueType, 4> queueTypes = {
			QueueType::Graphics,
			QueueType::Compute,
			QueueType::Transfer,
			QueueType::Present,
		};
		for (const QueueType type : queueTypes)
		{
		 const uint32_t i = static_cast<uint32_t>(type);
			const char *queueName = nullptr;
	
			switch (type)
			{
			case QueueType::Graphics:
				queueName = "Graphics Queue";
				break;
			case QueueType::Compute:
				queueName = "Compute Queue";
				break;
			case QueueType::Transfer:
				queueName = "Transfer Queue";
				break;
			case QueueType::Present:
				queueName = "Present Queue";
				break;
			default:
				queueName = "Unknown Queue";
				break;
			}
	
			// Create queue instance and store in m_GPUQueues and s_Regular
			AllocateQueue(type, m_Config.cmdListsPerQueue, queueName);
			SEDX_CORE_ASSERT(m_GPUQueues[i], "Failed to create Queue of type {}", QueueToString(static_cast<QueueType>(i)));
	
	
			SEDX_CORE_TRACE_TAG("QueueManager", "Created {} (family index: {})", queueName, QueueToString(static_cast<QueueType>(GetFamilyIndexByType(type))));
		}

		// Initialize the command pool for the graphics queue family
		SEDX_CORE_ASSERT(m_FamilyIndices.graphics != (std::numeric_limits<uint32_t>::max)(),
						 "Graphics queue family index is invalid; cannot create command pool");
		m_CmdPool = CommandPool(m_Device, m_FamilyIndices.graphics, CommandPoolType::Resettable);
		SEDX_CORE_TRACE_TAG("QueueManager", "Command pool created for graphics queue family {}", m_FamilyIndices.graphics);

		// Populate m_CmdLists - this is what NextCommandList() cycles through
		Ref<Queue>* graphicsQueue = GetQueue(QueueType::Graphics);
		SEDX_CORE_ASSERT(graphicsQueue && *graphicsQueue, "Graphics queue must be valid before creating command lists");

		for (size_t i = 0; i < m_CmdLists.size(); ++i)
		{
			const std::string name = "CmdList_" + std::to_string(i);
			m_CmdLists[i] = CreateRef<CommandList>(graphicsQueue->Get(), m_CmdPool, name.c_str());
			SEDX_CORE_ASSERT(m_CmdLists[i], "Failed to create CommandList at index {}", i);
			SEDX_CORE_TRACE_TAG("QueueManager", "Created {}", name);
		}

		SEDX_CORE_TRACE_TAG("QueueManager", "Queue Manager initialization complete");
	}
	
	QueueManager::~QueueManager()
	{
		WaitIdleAll(); // Ensure all queues are idle before destruction

		// Explicitly release command lists before destroying the pool and device.
		// Member destruction order (reverse-declaration) also guarantees this, but
		// being explicit here keeps the intent clear and avoids fragile ordering.
		for (auto &cmdList : m_CmdLists)
		{
			cmdList.Reset();
		}
		m_CmdPool.Destroy();

		m_Device.Reset();
		m_Device = nullptr;
	}
	
	void QueueManager::AllocateQueue(QueueType type, uint32_t preAllocCmdList, const char *name)
	{
		//SEDX_PROFILE_SCOPE("QueueManager::AllocateQueue");
	
		// Validate queue type
		const uint32_t typeIndex = static_cast<uint32_t>(type);
		if (typeIndex >= m_GPUQueues.size())
		{
			SEDX_CORE_ERROR_TAG("QueueManager", "Invalid queue type requested: {}", QueueToString(static_cast<QueueType>(typeIndex)));
		   return;
		}
	
		// Thread-safe allocation
		std::scoped_lock lock(s_MutexAllocation);

		// Check if queue already exists for this type
		if (m_GPUQueues[typeIndex])
		{
			SEDX_CORE_TRACE_TAG("QueueManager", "Returning existing queue for type {}", QueueToString(static_cast<QueueType>(typeIndex)));
			return;
		}
		// Determine queue name
		const char *queueName = name ? name : "Unnamed Queue";

		// Create new queue instance
		Ref<Queue> queue = CreateRef<Queue>(m_Device, type, queueName);  // Pass m_Device

		// Initialize the queue - retrieves VkQueue handle from device
		queue->Init();

		// Store the queue
		m_GPUQueues[typeIndex] = queue;
		s_Regular[typeIndex] = queue;

		//SEDX_CORE_TRACE_TAG("QueueManager", "Allocated {} with {} pre-allocated command lists", queueName, queue->GetPreAllocatedCmdLists());
	}
	
	void QueueManager::FreeQueue(Ref<Queue> queue)
	{
		if (!queue)
		{
			SEDX_CORE_WARN_TAG("QueueManager", "Attempted to free null queue");
			return;
		}
	
		//SEDX_PROFILE_SCOPE("QueueManager::FreeQueue");
	
		QueueType type = queue->GetType();
		const uint32_t typeIndex = static_cast<uint32_t>(type);
	
		// Validate queue type
		if (typeIndex >= m_GPUQueues.size())
		{
			SEDX_CORE_ERROR_TAG("QueueManager", "Invalid queue type for free: {}", QueueToString(static_cast<QueueType>(typeIndex)));
			return;
		}
	
		SEDX_CORE_TRACE_TAG("QueueManager", "Freeing queue of type {}", QueueToString(static_cast<QueueType>(typeIndex)));
	
		// Thread-safe deallocation
		std::scoped_lock lock(s_MutexAllocation);
	
		// Ensure queue is idle before freeing
		SEDX_CORE_TRACE_TAG("QueueManager", "Freeing queue of type {}...", QueueToString(static_cast<QueueType>(typeIndex)));
		Queue::WaitIdle(*queue);
	
		// Clear from tracked queues
		if (m_GPUQueues[typeIndex] == queue)
		{
			m_GPUQueues[typeIndex].Reset();
			m_GPUQueues[typeIndex] = nullptr;
	
			s_Regular[typeIndex].Reset();
			s_Regular[typeIndex] = nullptr;
		}
	
		// Queue object will be destroyed when last reference is released
		SEDX_CORE_TRACE_TAG("QueueManager", "Queue freed successfully");
	}
	
	void QueueManager::WaitIdleAll(const bool flush)
	{
		//SEDX_PROFILE_SCOPE("QueueManager::WaitIdleAll");
		SEDX_CORE_TRACE_TAG("QueueManager", "Waiting for all GPU queues to become idle...");
	
		// Thread-safe iteration with proper synchronization
		std::scoped_lock lock(s_MutexAllocation);
	
		// Iterate all GPU queues and wait for them to become idle
		uint32_t idleCount = 0;
		for (auto &queueRef : s_Regular)
		{
			if (queueRef)
			{
				// Flush pending work and wait for completion
				Queue::WaitIdle(*queueRef);
				idleCount++;
				SEDX_CORE_TRACE_TAG("QueueManager", "{} queue is now idle", QueueToString(queueRef->GetType()));
			}
		}
	
		SEDX_CORE_TRACE_TAG("QueueManager", "{} queues are now idle", idleCount);
	
		// After queues are idle it's safe to destroy thread-local command pools and
		// any Vulkan command pools owned by the CommandBuffer/CommandPool system.
		// This ensures no pending GPU work is using these resources.
	}

	Ref<Queue> *QueueManager::GetQueue(QueueType type)
	{
		const uint32_t idx = static_cast<uint32_t>(type);
		if (idx >= m_GPUQueues.size())
		{
			SEDX_CORE_WARN_TAG("QueueManager", "GetQueue called with out-of-range queue type {}", QueueToString(static_cast<QueueType>(idx)));
			return nullptr;
		}
	
		SEDX_CORE_TRACE_TAG("QueueManager", "Returning queue for type {}", QueueToString(static_cast<QueueType>(idx)));
		return &m_GPUQueues[idx];
	}
	
	uint32_t QueueManager::GetFamilyIndex(const Ref<Queue> &queue)
	{
		if (!queue)
		{
			SEDX_CORE_WARN_TAG("QueueManager", "GetFamilyIndex called with null queue");
			return (std::numeric_limits<uint32_t>::max)();
		}

		Ref<Device> device = RenderContext::Get()->GetDevice();
		if (!device)
		{
			SEDX_CORE_WARN_TAG("QueueManager", "Device not available");
			return (std::numeric_limits<uint32_t>::max)();
		}

		if (Ref<QueueManager> manager = device->GetQueueManager())
		{
			SEDX_CORE_TRACE_TAG("QueueManager", "Getting family index for queue type {}", QueueToString(queue->GetType()));
			return manager->GetFamilyIndexByType(queue->GetType());
		}

		SEDX_CORE_WARN_TAG("QueueManager", "GetFamilyIndex called for unallocated queue type {}", QueueToString(queue->GetType()));
		return (std::numeric_limits<uint32_t>::max)();
	}
	
	VkQueue QueueManager::GetQueueHandle(const Ref<Queue> &queue)
	{
		if (queue)
		{
			SEDX_CORE_TRACE_TAG("QueueManager", "Getting VkQueue handle for queue type {}", QueueToString(queue->GetType()));
			return static_cast<VkQueue>(Queue::GetQueueResource(queue->GetType()));
		}
	
		SEDX_CORE_WARN_TAG("QueueManager", "GetQueueHandle called with null queue");
		return VK_NULL_HANDLE;
	}
	
	const Ref<Queue> *QueueManager::GetQueue(QueueType type) const
	{
		const uint32_t idx = static_cast<uint32_t>(type);
		if (idx >= m_GPUQueues.size())
		{
			SEDX_CORE_WARN_TAG("QueueManager", "GetQueue called with out-of-range queue type {}",
				QueueToString(static_cast<QueueType>(idx)));

			return nullptr;
		}
	
		SEDX_CORE_TRACE_TAG("QueueManager", "Returning queue for type {}", QueueToString(static_cast<QueueType>(idx)));
		return &m_GPUQueues[idx];
	}
	
	VkQueue QueueManager::GetQueueHandleByType(QueueType type) const
	{
		if (const Ref<Queue> *queueRef = GetQueue(type); queueRef && *queueRef)
		{
			SEDX_CORE_TRACE_TAG("QueueManager", "Getting VkQueue handle for queue type {}", QueueToString(type));
			return static_cast<VkQueue>(Queue::GetQueueResource(type));
		}
	
		SEDX_CORE_WARN_TAG("QueueManager", "GetQueueHandleByType called for unallocated queue type {}", QueueToString(type));
		return VK_NULL_HANDLE;
	}
	
	uint32_t QueueManager::GetFamilyIndexByType(QueueType type) const
	{
		if (const Ref<Queue> *queueRef = GetQueue(type); queueRef && *queueRef)
		{
			switch (type)
			{
			case QueueType::Graphics:
			{
				SEDX_CORE_TRACE_TAG("QueueManager", "Getting family index for Graphics queue");
				return m_FamilyIndices.graphics;
			}
			case QueueType::Compute:
			{
				SEDX_CORE_TRACE_TAG("QueueManager", "Getting family index for Compute queue");
				return m_FamilyIndices.compute;
			}
			case QueueType::Transfer:
			{
				SEDX_CORE_TRACE_TAG("QueueManager", "Getting family index for Transfer queue");
				return m_FamilyIndices.transfer;
			}
			case QueueType::Present:
			{
				SEDX_CORE_TRACE_TAG("QueueManager", "Getting family index for Present queue");
				return m_FamilyIndices.present;
			}
			default:
			{
				SEDX_CORE_WARN_TAG("QueueManager", "GetFamilyIndexByType called with unknown queue type {}", static_cast<uint32_t>(type));
				break;
			}
			}
		}
	
		SEDX_CORE_WARN_TAG("QueueManager", "GetFamilyIndexByType called for unallocated queue type {}", static_cast<uint32_t>(type));
		return (std::numeric_limits<uint32_t>::max)();
	}
	
	void QueueManager::AddDeletionQueue(ResourceType type, void *resource)
	{
		AddDeletionQueue(type, resource, VK_NULL_HANDLE);
	}

	void QueueManager::AddDeletionQueue(ResourceType type, void *resource, VmaAllocation allocation)
	{
		if (!resource)
		{
			SEDX_CORE_WARN_TAG("QueueManager", "Attempted to add null resource to deletion queue");
			return;
		}
	
		std::scoped_lock guard(s_MutexDeletionQueue);
		s_DeletionQueue[type].emplace_back(DeletionQueueEntry{resource, allocation});
		SEDX_CORE_TRACE_TAG("QueueManager", "Added resource of type {} to deletion queue", static_cast<uint32_t>(type));
	}
	
	void QueueManager::ParseDeletionQueue()
	{
		Ref<Device> device = RenderContext::Get()->GetDevice();
		std::scoped_lock guard(s_MutexDeletionQueue);
	
		for (auto &it : s_DeletionQueue)
		{
			ResourceType type = it.first;
			for (const auto &entry : it.second)
			{
				void *resource = entry.resource;
				switch (type)
				{
				case ResourceType::Image: Buffer::FreeImageBuffer(static_cast<VkImage>(resource), entry.allocation);
					break;
				case ResourceType::ImageView:
					vkDestroyImageView(device->GetDevice(), static_cast<VkImageView>(resource), nullptr);
					break;
				case ResourceType::Sampler:
					vkDestroySampler(device->GetDevice(), reinterpret_cast<VkSampler>(resource), nullptr);
					break;
				case ResourceType::Buffer: Buffer::FreeBuffer(static_cast<VkBuffer>(resource), entry.allocation);
					break;
				case ResourceType::Shader:
					vkDestroyShaderModule(device->GetDevice(), static_cast<VkShaderModule>(resource), nullptr);
					break;
				case ResourceType::Semaphore:
					vkDestroySemaphore(device->GetDevice(), static_cast<VkSemaphore>(resource), nullptr);
					break;
				case ResourceType::Fence:
					vkDestroyFence(device->GetDevice(), static_cast<VkFence>(resource), nullptr);
					break;
				case ResourceType::DescriptorSetLayout:
					vkDestroyDescriptorSetLayout(device->GetDevice(), static_cast<VkDescriptorSetLayout>(resource), nullptr);
					break;
				case ResourceType::DescriptorPool:
					vkDestroyDescriptorPool(device->GetDevice(), static_cast<VkDescriptorPool>(resource), nullptr);
					break;
				case ResourceType::QueryPool:
					vkDestroyQueryPool(device->GetDevice(), static_cast<VkQueryPool>(resource), nullptr);
					break;
				case ResourceType::Pipeline:
					vkDestroyPipeline(device->GetDevice(), static_cast<VkPipeline>(resource), nullptr);
					break;
				case ResourceType::PipelineLayout:
					vkDestroyPipelineLayout(device->GetDevice(), static_cast<VkPipelineLayout>(resource), nullptr);
					break;
				case ResourceType::UniformBuffer:
					break;
				case ResourceType::UniformBufferSet:
					break;
				case ResourceType::AccelerationStructure: /*functions::destroy_acceleration_structure(device->GetDevice(), static_cast<VkAccelerationStructureKHR>(resource), nullptr);*/
					break;
				default:
					SEDX_CORE_ASSERT(false, "Unknown resource");
					break;
				case ResourceType::Unknown:
					break;
				case ResourceType::PhysicalDevice:
					break;
				case ResourceType::Device:
					break;
				case ResourceType::Queue:
					break;
				case ResourceType::CommandBuffer:
					break;
				case ResourceType::DeviceMemory:
					break;
				case ResourceType::Event:
					break;
				case ResourceType::PipelineCache:
					break;
				case ResourceType::RenderPass:
					break;
				case ResourceType::DescriptorSet:
					break;
				case ResourceType::CommandPool:
					break;
				case ResourceType::DebugCallback:
					break;
				case ResourceType::StorageBuffer:
					break;
				case ResourceType::StorageBufferSet:
					break;
				case ResourceType::Texture2D:
					break;
				case ResourceType::TextureCube:
					break;
				case ResourceType::Image2D:
					break;
				case ResourceType::CommandList:
					break;
				case ResourceType::MaxEnum:
					break;
				}
	
				/*
				// delete descriptor sets which are now invalid (because they are referring to a deleted resource)
				if (type == ResourceType::ImageView || type == ResourceType::Buffer)
				{
					for (auto it = Descriptor::sets.begin(); it != Descriptor::sets.end();)
					{
						if (it->second.IsReferingToResource(resource))
						{
							it = Descriptor::sets.erase(it);
							// ideally the descriptor set pool is not oblivious to the fact that we don't use this set anymore
							// maybe after a certain number of deletions we reset the entire pool to free memory
						}
						else
						{
							++it;
						}
					}
				}
				*/
	
				// samplers are bindless so they just update the set again
			}
		}
	
		SEDX_CORE_TRACE_TAG("QueueManager", "Cleared deletion queue after parsing");
		s_DeletionQueue.clear();
		device.Reset();
	}
	
	bool QueueManager::NeedToParseDeletionQueue()
	{
		static uint32_t framesEquilibrium = 0;
		static uint32_t objectsToDeletePrevious = 0;
	
		// count deletions in the queue
		uint32_t objectsToDelete = 0;
		for (uint32_t i = 0; i < static_cast<uint32_t>(ResourceType::MaxEnum); i++)
		{
			objectsToDelete += static_cast<uint32_t>(s_DeletionQueue[static_cast<ResourceType>(i)].size());
			SEDX_CORE_TRACE_TAG("QueueManager", "ResourceType {} has {} objects pending deletion", i, s_DeletionQueue[static_cast<ResourceType>(i)].size());
		}
	
		// check if the number of objects to delete has remained unchanged
		if (objectsToDelete > 0 && objectsToDelete == objectsToDeletePrevious)
		{
			framesEquilibrium++;
	
			// if it’s been stable for frame_self life frames, reset counter and delete
			if (framesEquilibrium >= 100) // Renderer resource frame lifetime
			{
				framesEquilibrium = 0;
				return true;
			}
	
			SEDX_CORE_TRACE_TAG("QueueManager", "Deletion queue stable for {} frames with {} objects pending deletion", framesEquilibrium, objectsToDelete);
		}
		else
		{
			SEDX_CORE_TRACE_TAG("QueueManager", "Deletion queue changed or empty at frame {} with {} objects pending deletion", framesEquilibrium, objectsToDelete);
			framesEquilibrium = 0; // Reset counter if the count changed or if nothing is in the queue
		}
	
		
		objectsToDeletePrevious = objectsToDelete; // Update the previous object count to the current count
		SEDX_CORE_TRACE_TAG("QueueManager", "Updated previous deletion count to {}", objectsToDeletePrevious);
	
		return false;
	}
	
	CommandList* QueueManager::NextCommandList()
	{
		m_Index = (m_Index + 1) % static_cast<uint32_t>(m_CmdLists.size());
		auto& cmdList = m_CmdLists[m_Index];

		SEDX_CORE_ASSERT(cmdList, "CommandList at index {} is null, m_CmdLists was not initialized", m_Index.load());

		// submit any pending work (toggling between fullscreen and windowed mode can leave work)
		if (cmdList->GetState() == CommandState::Recording)
		{
			cmdList->Submit(0, false);
		}

		// with enough command lists available, there is no wait time
		if (cmdList->GetState() == CommandState::Submitted)
		{
			cmdList->WaitForExecution();
		}

		SEDX_CORE_ASSERT(cmdList->GetState() == CommandState::Idle);

		return cmdList.Get();
	}

}

// -------------------------------------------------------
