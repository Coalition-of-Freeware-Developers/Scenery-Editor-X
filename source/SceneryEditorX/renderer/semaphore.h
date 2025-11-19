/**
* -------------------------------------------------------
* Scenery Editor X
* -------------------------------------------------------
* Copyright (c) 2025 Thomas Ray 
* Copyright (c) 2025 Coalition of Freeware Developers
* -------------------------------------------------------
* semaphore.h
* -------------------------------------------------------
* Created: 28/8/2025
* -------------------------------------------------------
*/
#pragma once
#include "vulkan/vk_enums.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	/**
	 * @class Semaphore
	 * @brief Wrapper for Vulkan semaphore synchronization primitives
	 * 
	 * Manages Vulkan semaphores (both binary and timeline) for GPU-GPU synchronization.
	 * Provides static factory methods and instance methods for semaphore operations.
	 * 
	 * @note This class stores a reference to the Vulkan semaphore handle but does not
	 *       own the resource - cleanup must be managed externally.
	 */
	class Semaphore : public RefCounted
	{
	public:
        /**
         * @brief Construct a semaphore wrapper from an existing resource handle
         * @param type Type of semaphore (binary or timeline)
         * @param resource Reference to the Vulkan semaphore handle (VkSemaphore)
         */
        explicit Semaphore(FrameSyncType type, void*& resource);
        virtual ~Semaphore() override = default;

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Static Factory Methods
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Create a new Vulkan semaphore
         * @param type Type of semaphore to create (binary or timeline)
         * @param resource Output parameter - receives the created VkSemaphore handle
         */
	    static void Create(FrameSyncType type, void*& resource);

        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
        /// Timeline Semaphore Operations
        /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

        /**
         * @brief Wait for a timeline semaphore to reach a specific value
         * @param value Signal value to wait for
         * @param timeout Timeout in nanoseconds (UINT64_MAX = wait indefinitely)
         * @param resource Reference to the timeline semaphore handle
         */
        static void WaitTime(uint64_t value, uint64_t timeout, void*& resource);
        
        /**
         * @brief Signal a timeline semaphore with a specific value
         * @param value Signal value to set
         * @param resource Reference to the timeline semaphore handle
         */
        static void Signal(uint64_t value, void*& resource);
        
        /**
         * @brief Get the current counter value of a timeline semaphore
         * @param resource Reference to the timeline semaphore handle
         * @return Current semaphore counter value
         */
        static uint64_t GetCounterValue(void*& resource);

        void* GetResource() const { return m_Resource; }

        FrameSyncType GetType() const { return m_Type; }

    private:

        void *m_Resource = nullptr; // Reference to the underlying Vulkan semaphore handle
        FrameSyncType m_Type = FrameSyncType::MaxEnum; // Type of semaphore (binary or timeline)
    };

}

// -------------------------------------------------------
