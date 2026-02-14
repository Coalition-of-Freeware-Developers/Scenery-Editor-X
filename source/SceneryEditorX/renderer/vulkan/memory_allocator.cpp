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
 * memory_allocator.cpp
 * -------------------------------------------------------
 * Created: 09/02/2026
 * -------------------------------------------------------
 */
#include "memory_allocator.h"
#include "render_context.h"
#include <mutex>

/// VMA implementation — must be defined in exactly ONE translation unit.
/// VK_NO_PROTOTYPES is active (volk is used), so we disable VMA's static
/// Vulkan function resolution and enable dynamic resolution via the
/// VmaVulkanFunctions struct populated in MemoryAllocator::Init().
#define VMA_IMPLEMENTATION
#define VMA_STATIC_VULKAN_FUNCTIONS  0
#define VMA_DYNAMIC_VULKAN_FUNCTIONS 1
#include <vma/vk_mem_alloc.h>
#include <volk/volk.h>

// -------------------------------------------------------

namespace SceneryEditorX
{

    static VmaAllocator s_Allocator;
    static std::mutex s_MutexAllocator;
    static std::unordered_map<void *, VmaAllocation> s_Allocations;

    // -------------------------------------------------------

    MemoryAllocator::MemoryAllocator()
    {
        m_Device = RenderContext::Get()->GetDevice();

    }

    MemoryAllocator::~MemoryAllocator()
    {
        Destroy();
        m_Device.Reset();
        m_Device = nullptr;
    }

    void MemoryAllocator::Init()
	{
        // Zero-initialize and provide the two root function pointers.
        // With VMA_DYNAMIC_VULKAN_FUNCTIONS=1, VMA will use these to
        // resolve all other Vulkan function pointers at runtime.
        VmaVulkanFunctions vkFunctions = {};
        vkFunctions.vkGetInstanceProcAddr = vkGetInstanceProcAddr;
        vkFunctions.vkGetDeviceProcAddr   = vkGetDeviceProcAddr;

        Ref<RenderContext> ctx = RenderContext::Get();
        VmaAllocatorCreateInfo allocatorInfo = {};
        allocatorInfo.physicalDevice = ctx->GetDevice()->GetPhysicalDevice();
        allocatorInfo.device = ctx->GetDevice()->GetLogicalDevice();
        allocatorInfo.instance = ctx->GetInstance();
        allocatorInfo.vulkanApiVersion = ctx->GetDevice()->GetDeviceProperties().apiVersion;
        allocatorInfo.pVulkanFunctions = &vkFunctions, 
        allocatorInfo.flags = VMA_ALLOCATOR_CREATE_EXT_MEMORY_BUDGET_BIT;
        if (Ref<Device> device = ctx->GetLogicalDevice(); Device::GetDeviceStatics().isRayTracingSupported)
        {
            allocatorInfo.flags |= VMA_ALLOCATOR_CREATE_BUFFER_DEVICE_ADDRESS_BIT;
        }

        SEDX_CORE_ASSERT(vmaCreateAllocator(&allocatorInfo, &s_Allocator));
	}

    /**
     * @brief Per-frame update for the device, used to manage memory allocation frames.
     * @param frameCount Current frame count
     */
    void MemoryAllocator::Tick(const uint64_t frameCount)
    {
        Ref<Device> device = RenderContext::Get()->GetDevice();
        /**
         * https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/staying_within_budget.html
         * make sure to call vmaSetCurrentFrameIndex() every frame
         * budget is queried from Vulkan inside of it to avoid overhead of querying it with every allocation
         */
        if (!device.IsValid())
        {
            SEDX_CORE_WARN_TAG("Device", "Device singleton is not initialized before Tick().");
            return;
        }

        vmaSetCurrentFrameIndex(MemoryAllocator::GetAllocator(), static_cast<uint32_t>(frameCount));
    }

	void MemoryAllocator::Destroy()
	{
        SEDX_CORE_ASSERT(s_Allocator != nullptr);
        SEDX_CORE_ASSERT(s_Allocations.empty(), "There are still allocations");
        vmaDestroyAllocator(s_Allocator);
        s_Allocator = nullptr;
	}

	void MemoryAllocator::SaveAllocation(void *resource, VmaAllocation allocation)
	{
        SEDX_CORE_ASSERT(resource != nullptr);
        std::scoped_lock lock(s_MutexAllocator);
        s_Allocations.emplace(resource, allocation);
	}

	void MemoryAllocator::FreeAllocation(void *resource)
	{
        std::scoped_lock lock(s_MutexAllocator);
        s_Allocations.erase(resource);
	}

	VmaAllocation MemoryAllocator::GetAllocation(void *resource)
    {
        std::scoped_lock lock(s_MutexAllocator);
        auto it = s_Allocations.find(resource);
        return it != s_Allocations.end() ? it->second : nullptr;
	}

    VmaAllocator MemoryAllocator::GetAllocator()
    {
        return s_Allocator;
    }

} // namespace SceneryEditorX

// -------------------------------------------------------
