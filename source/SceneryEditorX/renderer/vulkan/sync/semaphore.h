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
 * semaphore.h
 * -------------------------------------------------------
 * Created: 26/02/2026
 * -------------------------------------------------------
 */
#pragma once
#include "sync_object.h"
#include "SceneryEditorX/utils/inheritance.h"

// -------------------------------------------------------

namespace SceneryEditorX
{
	
    /**
     * @class Semaphore
     * @brief Represents a Vulkan semaphore used for synchronization between GPU operations.
     */
    class Semaphore : public SyncObject, public SharedObject
    {
    public:
        Semaphore();
        /**
         * @brief Constructs a Semaphore with an explicit synchronization type.
         * Pass SyncType::SemaphoreTimeline to create a Vulkan timeline semaphore.
         * @param type SyncType::Semaphore for binary, SyncType::SemaphoreTimeline for timeline.
         */
        explicit Semaphore(SyncType type);
        virtual ~Semaphore() override;

        void CreateSyncObject() override;
        void Wait(const uint64_t timeout) override;
        static void Signal(const uint64_t value, const VkSemaphore &semaphore);
        bool IsSignaled() override;
        void Destroy() override;

        static uint64_t GetValue(const VkSemaphore &semaphore);
        [[nodiscard]] VkSemaphore GetSemaphore() const { return m_Semaphore; }
    private:
		uint64_t m_Value = 0;
        SyncType m_Type = SyncType::Semaphore;
        VkSemaphore m_Semaphore = VK_NULL_HANDLE;
    };

}

// -------------------------------------------------------
