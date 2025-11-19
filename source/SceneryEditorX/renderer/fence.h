/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* fence.h
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#pragma once

// -------------------------------------------------------

namespace SceneryEditorX
{
    /**
     * @class Fence
     * @brief Wrapper for Vulkan fence synchronization primitives
     * 
     * Manages Vulkan fences for CPU-GPU synchronization. Fences allow the CPU
     * to wait for GPU work to complete before proceeding.
     * 
     * @note This class stores a reference to the Vulkan fence handle but does not
     *       own the resource - cleanup must be managed externally.
     */
    class Fence : public RefCounted
	{
	public:
        /**
         * @brief Construct a fence wrapper from an existing resource handle
         * @param resource Reference to the Vulkan fence handle (VkFence)
         */
	    explicit Fence(void*& resource);
        
        virtual ~Fence() override = default;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Static Factory Methods
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Create a new Vulkan fence
         * @param resource Output parameter - receives the created VkFence handle
         */
	    static void Create(void*& resource);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Fence Operations
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Check if the fence is currently signaled
         * @param resource Reference to the fence handle
         * @return True if signaled, false otherwise
         */
	    static bool IsSignaled(void*& resource);
        
        /**
         * @brief Wait for the fence to be signaled
         * @param timeout Timeout in nanoseconds (UINT64_MAX = wait indefinitely)
         * @param resource Reference to the fence handle
         */
	    static void WaitTime(uint64_t timeout, void*& resource);
        
        /**
         * @brief Reset the fence to unsignaled state
         * @param resource Reference to the fence handle
         */
	    static void Reset(void*& resource);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Resource Access
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Get the underlying Vulkan fence handle
         * @return Void pointer to VkFence (can be cast to VkFence)
         */
        void* GetRhiResource() const { return m_Resource; }

    private:
        // Reference to the underlying Vulkan fence handle
        void* m_Resource = nullptr;
	};

}

// -------------------------------------------------------
